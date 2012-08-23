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

#include "CallbackCategory.h"
#include "Scheduler.h"
#include "PowerdScheduler.h"
#include "Category.h"

const CallbackCategoryHandler::Method CallbackCategoryHandler::s_methods[] = {
	{ _T("scheduledwakeup"), (Callback) &CallbackCategoryHandler::ScheduledWakeup },
#ifdef UNITTEST
	{ _T("schedulertest"), (Callback) &CallbackCategoryHandler::SchedulerTest },
#endif /* UNITTEST */
	{ NULL, NULL }
};

MojLogger CallbackCategoryHandler::s_log(_T("activitymanager.callbackhandler"));

CallbackCategoryHandler::CallbackCategoryHandler(
	boost::shared_ptr<Scheduler> scheduler, MojService *service)
	: m_scheduler(scheduler)
	, m_service(service)
{
#ifdef UNITTEST
	m_testDriver = boost::make_shared<TestDriver>(m_service);
#endif
}

CallbackCategoryHandler::~CallbackCategoryHandler()
{
}

MojErr CallbackCategoryHandler::Init()
{
	MojErr err;

	err = addMethods(s_methods);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr
CallbackCategoryHandler::ScheduledWakeup(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Callback: ScheduledWakeup"));

	boost::dynamic_pointer_cast<PowerdScheduler,Scheduler>(m_scheduler)->ScheduledWakeup();

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

#ifdef UNITTEST

MojErr
CallbackCategoryHandler::SchedulerTest(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Callback: Scheduler Test Start: %s"),
		MojoObjectJson(payload).c_str());

	m_testDriver->Start(payload);

	MojErr err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

#endif
