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
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Preparing put command for [Activity %llu]"),
		activity->GetId());

	return boost::make_shared<MojoDBStoreCommand>(m_service,
		activity, completion);
}

boost::shared_ptr<PersistCommand>
MojoDBProxy::PrepareDeleteCommand(boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Preparing delete command for [Activity %llu]"),
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
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Loading persisted Activities from MojoDB"));

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
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Processing Activities loaded from MojoDB"));

	/* Don't allow the Activity Manager to start up if the MojoDB load
	 * fails ... */
	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogCritical(s_log, _T("Uncorrectable error loading Activities "
				"from MojoDB: %s"), MojoObjectJson(response).c_str());
#ifdef ACTIVITYMANAGER_REQUIRE_DB
			m_app->shutdown();
#else
			m_app->ready();
#endif
		} else {
			MojLogWarning(s_log, _T("Error loading Activities from MojoDB, "
				"retrying: %s"), MojoObjectJson(response).c_str());
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
				MojLogWarning(s_log, _T("activityId not found loading "
					"Activities"));
					continue;
			}

			MojString id;
			MojErr err = rep.get(_T("_id"), id, found);
			if (err) {
				MojLogError(s_log, _T("Error retrieving _id from results "
					"returned from MojoDB"));
				continue;
			}

			if (!found) {
				MojLogError(s_log, _T("_id not found loading Activities from "
					"MojoDB"));
				continue;
			}

			MojInt64 rev;
			found = rep.get(_T("_rev"), rev);
			if (!found) {
				MojLogError(s_log, _T("_rev not found loading Activities from "
					"MojoDB"));
				continue;
			}

			boost::shared_ptr<MojoDBPersistToken> pt =
				boost::make_shared<MojoDBPersistToken>(id, rev);

			boost::shared_ptr<Activity> act;

			try {
				act = m_json->CreateActivity(rep, Activity::PrivateBus, true);
			} catch (const std::exception& except) {
				MojLogError(s_log, _T("Exception \"%s\" decoding encoded "
					"Activity: %s"), except.what(),
					MojoObjectJson(rep).c_str());
				m_oldTokens.push_back(pt);
				continue;
			} catch (...) {
				MojLogError(s_log, _T("Unknown exception decoding encoded "
					"Activity: %s"), MojoObjectJson(rep).c_str());
				m_oldTokens.push_back(pt);
				continue;
			}

			act->SetPersistToken(pt);

			/* Attempt to register this Activity's Id and Name, in order. */

			try {
				m_am->RegisterActivityId(act);
			} catch (...) {
				MojLogError(s_log, _T("[Activity %llu] Failed to register ID"),
					act->GetId());

				/* Another Activity is already registered.  Determine which
				 * is newer, and kill the older one. */

				boost::shared_ptr<Activity> old = m_am->GetActivity(
					act->GetId());
				boost::shared_ptr<MojoDBPersistToken> oldPt =
					boost::dynamic_pointer_cast<MojoDBPersistToken,
						PersistToken>(old->GetPersistToken());

				if (pt->GetRev() > oldPt->GetRev()) {
					MojLogWarning(s_log, _T("[Activity %llu] revision %llu "
						"replacing [Activity %llu] revision %llu"),
						act->GetId(), (unsigned long long)pt->GetRev(),
						old->GetId(), (unsigned long long)oldPt->GetRev());

					m_oldTokens.push_back(oldPt);
					m_am->UnregisterActivityName(old);
					m_am->ReleaseActivity(old);

					m_am->RegisterActivityId(act);
				} else {
					MojLogWarning(s_log, _T("[Activity %llu] revision %llu "
						"not replacing [Activity %llu] revision %llu"),
						act->GetId(), (unsigned long long)pt->GetRev(),
						old->GetId(), (unsigned long long)oldPt->GetRev());

					m_oldTokens.push_back(pt);
					m_am->ReleaseActivity(act);
					continue;
				}
			}

			try {
				m_am->RegisterActivityName(act);
			} catch (...) {
				MojLogError(s_log, _T("[Activity %llu] Failed to register as "
					"%s/\"%s\""), act->GetId(),
					act->GetCreator().GetString().c_str(),
					act->GetName().c_str());

				/* Another Activity is already registered.  Determine which
				 * is newer, and kill the older one. */

				boost::shared_ptr<Activity> old = m_am->GetActivity(
					act->GetName(), act->GetCreator());
				boost::shared_ptr<MojoDBPersistToken> oldPt =
					boost::dynamic_pointer_cast<MojoDBPersistToken,
						PersistToken>(old->GetPersistToken());

				if (pt->GetRev() > oldPt->GetRev()) {
					MojLogWarning(s_log, _T("[Activity %llu] revision %llu "
						"replacing [Activity %llu] revision %llu"),
						act->GetId(), (unsigned long long)pt->GetRev(),
						old->GetId(), (unsigned long long)oldPt->GetRev());

					m_oldTokens.push_back(oldPt);
					m_am->UnregisterActivityName(old);
					m_am->ReleaseActivity(old);

					m_am->RegisterActivityName(act);
				} else {
					MojLogWarning(s_log, _T("[Activity %llu] revision %llu "
						"not replacing [Activity %llu] revision %llu"),
						act->GetId(), (unsigned long long)pt->GetRev(),
						old->GetId(), (unsigned long long)oldPt->GetRev());

					m_oldTokens.push_back(pt);
					m_am->ReleaseActivity(act);
					continue;
				}
			}

			MojLogNotice(s_log, _T("[Activity %llu] (\"%s\"): _id %s, rev %llu "
				"loaded"), act->GetId(), act->GetName().c_str(), id.data(),
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
		MojLogError(s_log, _T("Error getting page parameter in MojoDB query "
			"response"));
		return;
	}

	if (found) {
		MojLogInfo(s_log, _T("Preparing to request next page (\"%s\") of "
			"Activities"), page.data());

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
		MojLogNotice(s_log, _T("All Activities successfully loaded from "
			"MojoDB"));

		if (!m_oldTokens.empty()) {
			MojLogNotice(s_log, _T("Beginning purge of old Activities from "
				"database"));
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
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Purge of batch of old Activities complete"));

	/* If there was a transient error, re-call */
	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("Purge of batch of old Activities failed: "
				"%s"), MojoObjectJson(response).c_str());
			m_call.reset();
		} else {
			MojLogWarning(s_log, _T("Purge of batch of old Activities failed, "
				"retrying: %s"), MojoObjectJson(response).c_str());
			m_call->Call();
		}
		return;
	}

	if (!m_oldTokens.empty()) {
		PreparePurgeCall();
		m_call->Call();
	} else {
		MojLogInfo(s_log, _T("Done purging old Activities"));
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
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Load of static Activity configuration complete"));

	if (err != MojErrNone) {
		MojLogWarning(s_log, _T("Failed to load static Activity configuration: "
			"%s"), MojoObjectJson(response).c_str());
	}

	m_call.reset();
	m_am->Enable(ActivityManager::CONFIGURATION_LOADED);
}

void MojoDBProxy::PreparePurgeCall()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Preparing to purge batch of old Activities"));

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
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Prepared to update Activity configuration from "
		"static store"));

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

