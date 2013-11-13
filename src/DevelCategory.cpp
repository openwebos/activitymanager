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

#include "DevelCategory.h"
#include "Category.h"
#include "ActivityManager.h"
#include "MojoJsonConverter.h"
#include "MojoSubscription.h"
#include "Activity.h"
#include "ResourceManager.h"
#include "ContainerManager.h"
#include "Logging.h"

/*!
 * \page com_palm_activitymanager_devel Service API com.palm.activitymanager/devel/
 * Private methods:
 * - \ref com_palm_activitymanager_devel_evict
 * - \ref com_palm_activitymanager_devel_run
 * - \ref com_palm_activitymanager_devel_concurrency
 * - \ref com_palm_activitymanager_devel_priority_control
 */

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

/*!
\page com_palm_activitymanager_devel
\n
\section com_palm_activitymanager_devel_evict evict

\e Private.

com.palm.activitymanager/devel/evict

Evict one or all Activities from the background queue.

\subsection com_palm_activitymanager_devel_evict_syntax Syntax:
\code
{
    "activityId": int,
    "evictAll": boolean
}
\endcode

\param activityId The ID of the background activity to evict. Either this or \e
                  evictAll is required.
\param evictAll Set to true to evict all Activities from the background queue.
                Either this or \e activityId is required.

\subsection com_palm_activitymanager_devel_evict_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_devel_evict_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.activitymanager/devel/evict '{ "evictAll": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": -1000,
    "errorText": "Activity not on background queue",
    "returnValue": false
}
\endcode
*/

MojErr
DevelCategoryHandler::Evict(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Evict: %s", MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	bool evictAll = false;

	payload.get(_T("evictAll"), evictAll);

	if (evictAll) {
		LOG_DEBUG("EVICTING ALL ACTIVITIES FROM BACKGROUND QUEUE");
		m_am->EvictAllBackgroundActivities();
	} else {
		boost::shared_ptr<Activity> act;

		err = LookupActivity(msg, payload, act);
		MojErrCheck(err);

		LOG_DEBUG(" EVICTING [Activity %llu] FROM BACKGROUND QUEUE",act->GetId());

		m_am->EvictBackgroundActivity(act);
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager_devel
\n
\section com_palm_activitymanager_devel_run run

\e Private.

com.palm.activitymanager/devel/run

Run one or all Activities that are in ready queue.

\subsection com_palm_activitymanager_devel_run_syntax Syntax:
\code
{
    "activityId": int,
    "runAll": boolean
}
\endcode

\param activityId The ID of the ready activity to run. Either this or \e runAll
                  is required.
\param runAll Set to true to run all ready Activities. Either this or \e
              activityId is required.

\subsection com_palm_activitymanager_devel_run_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_devel_run_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.activitymanager/devel/run '{ "runAll": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": -1000,
    "errorText": "Activity not on ready queue",
    "returnValue": false
}
\endcode
*/

MojErr
DevelCategoryHandler::Run(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Run: %s", MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	bool runAll = false;

	payload.get(_T("runAll"), runAll);

	if (runAll) {
		LOG_DEBUG(" EVICTING ALL ACTIVITIES FROM BACKGROUND QUEUE");

		m_am->RunAllReadyActivities();
	} else {
		boost::shared_ptr<Activity> act;

		err = LookupActivity(msg, payload, act);
		MojErrCheck(err);

		LOG_DEBUG("EVICTING [Activity %llu] FROM BACKGROUND QUEUE", act->GetId());

		m_am->RunReadyBackgroundActivity(act);
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager_devel
\n
\section com_palm_activitymanager_devel_concurrency concurrency

\e Private.

com.palm.activitymanager/devel/concurrency

Set background Activity concurrency level for the Activity Manager.

\subsection com_palm_activitymanager_devel_concurrency_syntax Syntax:
\code
{
    "level": int,
    "unlimited": boolean
}
\endcode

\param level Number of concurrent background Activities. Either this is
             required, or \e unlimited must be set to true.
\param unlimited Set to true to allow unlimited amount of concurrent background
                 Activities. This must be true or \e level needs to be
                 specified.

\subsection com_palm_activitymanager_devel_concurrency_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_devel_concurrency_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.activitymanager/devel/concurrency '{ "unlimited": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 22,
    "errorText": "Either \"unlimited\":true, or \"level\":<number concurrent Activities> must be specified",
    "returnValue": false
}
\endcode
*/

MojErr
DevelCategoryHandler::SetConcurrency(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("SetConcurrency: %s",
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
		LOG_DEBUG("Attempt to set background concurrency did not specify \"unlimited\":true or a \"level\"");
		err = msg->replyError(MojErrInvalidArg, "Either \"unlimited\":true, "
			"or \"level\":<number concurrent Activities> must be specified");
		MojErrCheck(err);
	}

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager_devel
\n
\section com_palm_activitymanager_devel_priority_control priorityControl

\e Private.

com.palm.activitymanager/devel/priorityControl

Enable or disable priority control.

\subsection com_palm_activitymanager_devel_priority_control_syntax Syntax:
\code
{
    "enabled": boolean
}
\endcode

\param enabled Flag to enable or disable priority control.

\subsection com_palm_activitymanager_devel_priority_control_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_devel_priority_control_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.activitymanager/devel/priorityControl '{ "enabled": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 22,
    "errorText": "Must specify \"enabled\":true or \"enabled\":false",
    "returnValue": false
}
\endcode
*/

MojErr
DevelCategoryHandler::PriorityControl(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("PriorityControl: %s",
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
		MojErr err = msg->replyError(MojErrNotFound, except.what());
		MojErrCheck(err);
		return MojErrNotFound;
	}

	return MojErrNone;
}

