// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "DevelCategory.h"
#include "Category.h"
#include "ActivityManager.h"
#include "MojoJsonConverter.h"
#include "MojoSubscription.h"
#include "Activity.h"
#include "ResourceManager.h"
#include "ContainerManager.h"

const DevelCategoryHandler::Method DevelCategoryHandler::s_methods[] = {
	{ _T("evict"), (Callback) &DevelCategoryHandler::Evict },
	{ _T("run"), (Callback) &DevelCategoryHandler::Run },
	{ _T("concurrency"), (Callback) &DevelCategoryHandler::SetConcurrency },
	{ _T("priorityControl"), (Callback) &DevelCategoryHandler::PriorityControl },
	{ NULL, NULL }
};

MojLogger DevelCategoryHandler::s_log(_T("activitymanager.develhandler"));

DevelCategoryHandler::DevelCategoryHandler(
	boost::shared_ptr<ActivityManager> am,
	boost::shared_ptr<MojoJsonConverter> json,
	boost::shared_ptr<MasterResourceManager> resourceManager,
	boost::shared_ptr<ContainerManager> containerManager)
	: m_am(am)
	, m_json(json)
	, m_resourceManager(resourceManager)
	, m_containerManager(containerManager)
{
}

DevelCategoryHandler::~DevelCategoryHandler()
{
}

MojErr DevelCategoryHandler::Init()
{
	MojErr err;

	err = addMethods(s_methods);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr
DevelCategoryHandler::Evict(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Evict: %s"), MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	bool evictAll = false;

	payload.get(_T("evictAll"), evictAll);

	if (evictAll) {
		MojLogCritical(s_log, _T("EVICTING ALL ACTIVITIES FROM BACKGROUND "
			"QUEUE"));
		m_am->EvictAllBackgroundActivities();
	} else {
		boost::shared_ptr<Activity> act;

		err = LookupActivity(msg, payload, act);
		MojErrCheck(err);

		MojLogCritical(s_log, _T("EVICTING [Activity %llu] FROM BACKGROUND "
			"QUEUE"), act->GetId());

		m_am->EvictBackgroundActivity(act);
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

MojErr
DevelCategoryHandler::Run(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Run: %s"), MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	bool runAll = false;

	payload.get(_T("runAll"), runAll);

	if (runAll) {
		MojLogCritical(s_log, _T("EVICTING ALL ACTIVITIES FROM BACKGROUND "
			"QUEUE"));
		m_am->RunAllReadyActivities();
	} else {
		boost::shared_ptr<Activity> act;

		err = LookupActivity(msg, payload, act);
		MojErrCheck(err);

		MojLogCritical(s_log, _T("EVICTING [Activity %llu] FROM BACKGROUND "
			"QUEUE"), act->GetId());

		m_am->RunReadyBackgroundActivity(act);
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

MojErr
DevelCategoryHandler::SetConcurrency(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("SetConcurrency: %s"),
		MojoObjectJson(payload).c_str());

	bool unlimited = false;
	payload.get(_T("unlimited"), unlimited);

	MojUInt32 level;
	bool found = false;

	MojErr err = payload.get(_T("level"), level, found);
	MojErrCheck(err);

	if (unlimited || found) {
		m_am->SetBackgroundConcurrencyLevel(unlimited ?
			ActivityManager::UnlimitedBackgroundConcurrency :
			(unsigned int)level);

		err = msg->replySuccess();
		MojErrCheck(err);
	} else {
		MojLogWarning(s_log, _T("Attempt to set background concurrency did "
			"not specify \"unlimited\":true or a \"level\""));
		err = msg->replyError(MojErrInvalidArg, "Either \"unlimited\":true, "
			"or \"level\":<number concurrent Activities> must be specified");
		MojErrCheck(err);
	}

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

MojErr
DevelCategoryHandler::PriorityControl(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("PriorityControl: %s"),
		MojoObjectJson(payload).c_str());

	MojErr err;

	bool enabled = false;
	bool found = payload.get(_T("enabled"), enabled);
	if (!found) {
		err = msg->replyError(MojErrInvalidArg, _T("Must specify "
			"\"enabled\":true or \"enabled\":false"));
		MojErrCheck(err);
		return MojErrNone;
	}

	if (enabled) {
		m_resourceManager->Enable();
	} else {
		m_resourceManager->Disable();
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

MojErr
DevelCategoryHandler::LookupActivity(MojServiceMessage *msg, MojObject& payload, boost::shared_ptr<Activity>& act)
{
	try {
		act = m_json->LookupActivity(payload, MojoSubscription::GetBusId(msg));
	} catch (const std::exception& except) {
		msg->replyError(MojErrNotFound, except.what());
		return MojErrNotFound;
	}

	return MojErrNone;
}

