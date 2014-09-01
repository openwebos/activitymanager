// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// LICENSE@@@

#include "MojoDBProxy.h"
#include "MojoDBPersistToken.h"
#include "MojoDBPersistCommand.h"
#include "MojoJsonConverter.h"
#include "MojoCall.h"
#include "ActivityJson.h"
#include "Activity.h"
#include "ActivityManager.h"
#include "ServiceApp.h"
#include "Logging.h"

#include <stdexcept>
#include <core/MojObject.h>
#include <db/MojDbQuery.h>

const char *MojoDBProxy::ActivityKind = _T("com.palm.activity:1");

const int MojoDBProxy::PurgeBatchSize = MojDbQuery::MaxQueryLimit;

MojLogger MojoDBProxy::s_log(_T("activitymanager.mojodb"));

MojoDBProxy::MojoDBProxy(ActivityManagerApp *app, MojService *service,
	boost::shared_ptr<ActivityManager> am,
	boost::shared_ptr<MojoJsonConverter> json)
	: m_app(app)
	, m_service(service)
	, m_am(am)
	, m_json(json)
{
}

MojoDBProxy::~MojoDBProxy()
{
}

boost::shared_ptr<PersistCommand>
MojoDBProxy::PrepareStoreCommand(boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Preparing put command for [Activity %llu]",
		activity->GetId());

	return boost::make_shared<MojoDBStoreCommand>(m_service,
		activity, completion);
}

boost::shared_ptr<PersistCommand>
MojoDBProxy::PrepareDeleteCommand(boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Preparing delete command for [Activity %llu]",
		activity->GetId());

	return boost::make_shared<MojoDBDeleteCommand>(m_service,
		activity, completion);
}

boost::shared_ptr<PersistToken>
MojoDBProxy::CreateToken()
{
	return boost::make_shared<MojoDBPersistToken>();
}

void MojoDBProxy::LoadActivities()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Loading persisted Activities from MojoDB");

	MojObject query;
	query.putString(_T("from"), ActivityKind);

	MojObject params;
	params.put(_T("query"), query);

	m_call = boost::make_shared<MojoWeakPtrCall<MojoDBProxy> >(
		boost::dynamic_pointer_cast<MojoDBProxy, PersistProxy>
			(shared_from_this()),
		&MojoDBProxy::ActivityLoadResults,
		m_service, "palm://com.palm.db/find", params);

	m_call->Call();
}


void MojoDBProxy::ActivityLoadResults(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Processing Activities loaded from MojoDB");

	/* Don't allow the Activity Manager to start up if the MojoDB load
	 * fails ... */
	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			LOG_AM_ERROR(MSGID_LOAD_ACTIVITIES_FROM_DB_FAIL, 0,
				     "Uncorrectable error loading Activities from MojoDB: %s", MojoObjectJson(response).c_str());
#ifdef ACTIVITYMANAGER_REQUIRE_DB
			m_app->shutdown();
#else
			m_app->ready();
#endif
		} else {
			LOG_AM_WARNING(MSGID_ACTIVITIES_LOAD_ERR, 0, "Error loading Activities from MojoDB, retrying: %s",
                                    MojoObjectJson(response).c_str());
			m_call->Call();
		}
		return;
	}

	/* Clear current call */
	m_call.reset();

	bool found;
	MojErr err2;
	MojObject results;
	found = response.get(_T("results"), results);

	if (found) {
		for (MojObject::ConstArrayIterator iter = results.arrayBegin();
			iter != results.arrayEnd(); ++iter) {
			const MojObject& rep = *iter;
			MojInt64 activityId;

			bool found;
			found = rep.get(_T("activityId"), activityId);
			if (!found) {
				LOG_AM_WARNING(MSGID_ACTIVITYID_NOT_FOUND, 0, "activityId not found loading Activities");
					continue;
			}

			MojString id;
			MojErr err = rep.get(_T("_id"), id, found);
			if (err) {
				LOG_AM_WARNING(MSGID_RETRIEVE_ID_FAIL, 0, "Error retrieving _id from results returned from MojoDB");
				continue;
			}

			if (!found) {
				LOG_AM_WARNING(MSGID_ID_NOT_FOUND, 0, "_id not found loading Activities from MojoDB");
				continue;
			}

			MojInt64 rev;
			found = rep.get(_T("_rev"), rev);
			if (!found) {
				LOG_AM_WARNING(MSGID_REV_NOT_FOUND, 0, "_rev not found loading Activities from MojoDB");
				continue;
			}

			boost::shared_ptr<MojoDBPersistToken> pt =
				boost::make_shared<MojoDBPersistToken>(id, rev);

			boost::shared_ptr<Activity> act;

			try {
				act = m_json->CreateActivity(rep, Activity::PrivateBus, true);
			} catch (const std::exception& except) {
				LOG_AM_WARNING(MSGID_CREATE_ACTIVITY_EXCEPTION, 1, PMLOGKS("Exception",except.what()),
					  "Activity: %s", MojoObjectJson(rep).c_str());
				m_oldTokens.push_back(pt);
				continue;
			} catch (...) {
				LOG_AM_WARNING(MSGID_UNKNOWN_EXCEPTION, 0, "Activity : %s. Unknown exception decoding encoded",
					  MojoObjectJson(rep).c_str());
				m_oldTokens.push_back(pt);
				continue;
			}

			act->SetPersistToken(pt);

			/* Attempt to register this Activity's Id and Name, in order. */

			try {
				m_am->RegisterActivityId(act);
			} catch (...) {
				LOG_AM_ERROR(MSGID_ACTIVITY_ID_REG_FAIL, 1, PMLOGKFV("Activity","%llu", act->GetId()), "");

				/* Another Activity is already registered.  Determine which
				 * is newer, and kill the older one. */

				boost::shared_ptr<Activity> old = m_am->GetActivity(
					act->GetId());
				boost::shared_ptr<MojoDBPersistToken> oldPt =
					boost::dynamic_pointer_cast<MojoDBPersistToken,
						PersistToken>(old->GetPersistToken());

				if (pt->GetRev() > oldPt->GetRev()) {
					LOG_AM_WARNING(MSGID_ACTIVITY_REPLACED, 4, PMLOGKFV("Activity","%llu",act->GetId()),
						    PMLOGKFV("revision","%llu",(unsigned long long)pt->GetRev()),
						    PMLOGKFV("old_Activity","%llu",old->GetId()),
						    PMLOGKFV("old_revision","%llu",(unsigned long long)oldPt->GetRev()), "");

					m_oldTokens.push_back(oldPt);
					m_am->UnregisterActivityName(old);
					m_am->ReleaseActivity(old);

					m_am->RegisterActivityId(act);
				} else {
					LOG_AM_WARNING(MSGID_ACTIVITY_NOT_REPLACED, 4, PMLOGKFV("Activity","%llu",act->GetId()),
						    PMLOGKFV("revision","%llu",(unsigned long long)pt->GetRev()),
						    PMLOGKFV("old_Activity","%llu",old->GetId()),
						    PMLOGKFV("old_revision","%llu",(unsigned long long)oldPt->GetRev()), "");

					m_oldTokens.push_back(pt);
					m_am->ReleaseActivity(act);
					continue;
				}
			}

			try {
				m_am->RegisterActivityName(act);
			} catch (...) {
				LOG_AM_ERROR(MSGID_ACTIVITY_NAME_REG_FAIL, 3, PMLOGKFV("Activity","%llu",act->GetId()),
					  PMLOGKS("Creator_name",act->GetCreator().GetString().c_str()),
					  PMLOGKS("Register_name",act->GetName().c_str()), "");

				/* Another Activity is already registered.  Determine which
				 * is newer, and kill the older one. */

				boost::shared_ptr<Activity> old = m_am->GetActivity(
					act->GetName(), act->GetCreator());
				boost::shared_ptr<MojoDBPersistToken> oldPt =
					boost::dynamic_pointer_cast<MojoDBPersistToken,
						PersistToken>(old->GetPersistToken());

				if (pt->GetRev() > oldPt->GetRev()) {
					LOG_AM_WARNING(MSGID_ACTIVITY_REPLACED, 4, PMLOGKFV("Activity","%llu",act->GetId()),
						    PMLOGKFV("revision","%llu",(unsigned long long)pt->GetRev()),
						    PMLOGKFV("old_Activity","%llu",old->GetId()),
						    PMLOGKFV("old_revision","%llu",(unsigned long long)oldPt->GetRev()), "");

					m_oldTokens.push_back(oldPt);
					m_am->UnregisterActivityName(old);
					m_am->ReleaseActivity(old);

					m_am->RegisterActivityName(act);
				} else {
					LOG_AM_WARNING(MSGID_ACTIVITY_NOT_REPLACED, 4, PMLOGKFV("Activity","%llu",act->GetId()),
						    PMLOGKFV("revision","%llu",(unsigned long long)pt->GetRev()),
						    PMLOGKFV("old_Activity","%llu",old->GetId()),
						    PMLOGKFV("old_revision","%llu",(unsigned long long)oldPt->GetRev()), "");

					m_oldTokens.push_back(pt);
					m_am->ReleaseActivity(act);
					continue;
				}
			}

			LOG_AM_DEBUG("[Activity %llu] (\"%s\"): _id %s, rev %llu loaded",
				act->GetId(), act->GetName().c_str(), id.data(),
				(unsigned long long)rev);

			/* Request Activity be scheduled.  It won't transition to running
			 * until after the MojoDB load finishes (and the Activity Manager
			 * moves to the ready() and start()ed states). */
			m_am->StartActivity(act);
		}
	}

	MojString page;
	err2 = response.get(_T("page"), page, found);
	if (err2) {
		LOG_AM_ERROR(MSGID_GET_PAGE_FAIL, 0, "Error getting page parameter in MojoDB query response");
		return;
	}

	if (found) {
		LOG_AM_DEBUG("Preparing to request next page (\"%s\") of Activities",
			page.data());

		MojObject query;
		query.putString(_T("from"), ActivityKind);

		query.putString(_T("page"), page);

		MojObject params;
		params.put(_T("query"), query);

		m_call = boost::make_shared<MojoWeakPtrCall<MojoDBProxy> >(
			boost::dynamic_pointer_cast<MojoDBProxy, PersistProxy>
				(shared_from_this()),
			&MojoDBProxy::ActivityLoadResults,
			m_service, "palm://com.palm.db/find", params);

		m_call->Call();
	} else {
		LOG_AM_DEBUG("All Activities successfully loaded from MojoDB");

		if (!m_oldTokens.empty()) {
			LOG_AM_DEBUG("Beginning purge of old Activities from database");
			PreparePurgeCall();
			m_call->Call();
		} else {
#ifdef ACTIVITYMANAGER_CALL_CONFIGURATOR
			PrepareConfiguratorCall();
			m_call->Call();
#else
			m_call.reset();
			m_am->Enable(ActivityManager::CONFIGURATION_LOADED);
#endif
		}

		m_app->ready();
	}
}

void MojoDBProxy::ActivityPurgeComplete(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Purge of batch of old Activities complete");

	/* If there was a transient error, re-call */
	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			LOG_AM_ERROR(MSGID_ACTIVITIES_PURGE_FAILED, 0, "Purge of batch of old Activities failed: %s",
				  MojoObjectJson(response).c_str());
			m_call.reset();
		} else {
			LOG_AM_WARNING(MSGID_RETRY_ACTIVITY_PURGE, 0, "Purge of batch of old Activities failed, retrying: %s",
				    MojoObjectJson(response).c_str());
			m_call->Call();
		}
		return;
	}

	if (!m_oldTokens.empty()) {
		PreparePurgeCall();
		m_call->Call();
	} else {
		LOG_AM_DEBUG("Done purging old Activities");
#ifdef ACTIVITYMANAGER_CALL_CONFIGURATOR
		PrepareConfiguratorCall();
		m_call->Call();
#else
		m_call.reset();
		m_am->Enable(ActivityManager::CONFIGURATION_LOADED);
#endif
	}
}

void MojoDBProxy::ActivityConfiguratorComplete(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Load of static Activity configuration complete");

	if (err != MojErrNone) {
		LOG_AM_WARNING(MSGID_ACTIVITY_CONFIG_LOAD_FAIL, 0, "Failed to load static Activity configuration: %s",
			    MojoObjectJson(response).c_str());
	}

	m_call.reset();
	m_am->Enable(ActivityManager::CONFIGURATION_LOADED);
}

void MojoDBProxy::PreparePurgeCall()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Preparing to purge batch of old Activities");

	MojObject ids(MojObject::TypeArray);
	PopulatePurgeIds(ids);

	MojObject params;
	params.put(_T("ids"), ids);

	m_call = boost::make_shared<MojoWeakPtrCall<MojoDBProxy> >(
		boost::dynamic_pointer_cast<MojoDBProxy, PersistProxy>
			(shared_from_this()),
		&MojoDBProxy::ActivityPurgeComplete,
		m_service, "palm://com.palm.db/del", params);
}

void MojoDBProxy::PrepareConfiguratorCall()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Prepared to update Activity configuration from static store");

	MojObject types(MojObject::TypeArray);

	types.pushString(_T("activities"));

	MojObject params;
	params.put(_T("types"), types);

	m_call = boost::make_shared<MojoWeakPtrCall<MojoDBProxy> >(
		boost::dynamic_pointer_cast<MojoDBProxy, PersistProxy>
			(shared_from_this()),
		&MojoDBProxy::ActivityConfiguratorComplete,
		m_service, "palm://com.palm.configurator/run", params);
}

void MojoDBProxy::PopulatePurgeIds(MojObject& ids)
{
	int count = PurgeBatchSize;

	while (!m_oldTokens.empty()) {
		boost::shared_ptr<MojoDBPersistToken> pt = m_oldTokens.front();
		m_oldTokens.pop_front();

		ids.push(pt->GetId());
		if (--count == 0)
			return;
	}
}

