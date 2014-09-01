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

#include "CallbackCategory.h"
#include "Scheduler.h"
#include "PowerdScheduler.h"
#include "Category.h"
#include "Logging.h"

// TODO: I could not get this method to do anything, so leaving out of the generated documentation
/* !
 * \page com_palm_activitymanager_callback Service API com.palm.activitymanager/callback/
 * Private methods:
 * - \ref com_palm_activitymanager_callback_scheduledwakeup
 */

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

/* !
\page com_palm_activitymanager_callback
\n
\section com_palm_activitymanager_callback_scheduledwakeup scheduledwakeup

\e Private.

com.palm.activitymanager/callback/scheduledwakeup



\subsection com_palm_activitymanager_callback_scheduledwakeup_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_callback_scheduledwakeup_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_callback_scheduledwakeup_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/callback/scheduledwakeup '{ }'
\endcode

Example response for a succesful call:
\code
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
CallbackCategoryHandler::ScheduledWakeup(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Callback: ScheduledWakeup");

	boost::dynamic_pointer_cast<PowerdScheduler,Scheduler>(m_scheduler)->ScheduledWakeup();

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

#ifdef UNITTEST

MojErr
CallbackCategoryHandler::SchedulerTest(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Callback: Scheduler Test Start: %s",
		MojoObjectJson(payload).c_str());

	m_testDriver->Start(payload);

	MojErr err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

#endif
