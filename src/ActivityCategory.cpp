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

#include "Activity.h"
#include "Callback.h"
#include "ActivityManager.h"
#include "PowerManager.h"
#include "MojoTriggerManager.h"
#include "MojoJsonConverter.h"
#include "ActivityCategory.h"
#include "Subscription.h"
#include "MojoSubscription.h"
#include "Category.h"
#include "ActivityJson.h"
#include "PersistProxy.h"
#include "MojoPersistCommand.h"
#include "Completion.h"
#include "ResourceManager.h"
#include "ContainerManager.h"

#ifdef ACTIVITYMANAGER_USE_PUBLIC_BUS
#include <luna/MojLunaMessage.h>
#endif

#include <stdexcept>

/*!
 * \page com_palm_activitymanager Service API com.palm.activitymanager/
 * Public methods:
 * - \ref com_palm_activitymanager_create
 * - \ref com_palm_activitymanager_monitor
 * - \ref com_palm_activitymanager_join
 * - \ref com_palm_activitymanager_release
 * - \ref com_palm_activitymanager_adopt
 * - \ref com_palm_activitymanager_complete
 * - \ref com_palm_activitymanager_schedule
 * - \ref com_palm_activitymanager_start
 * - \ref com_palm_activitymanager_stop
 * - \ref com_palm_activitymanager_cancel
 * - \ref com_palm_activitymanager_pause
 * - \ref com_palm_activitymanager_focus
 * - \ref com_palm_activitymanager_unfocus
 * - \ref com_palm_activitymanager_addfocus
 * - \ref com_palm_activitymanager_list
 * - \ref com_palm_activitymanager_get_details
 * - \ref com_palm_activitymanager_info
 *
 * Private methods:
 * - \ref com_palm_activitymanager_map_process
 * - \ref com_palm_activitymanager_enable
 * - \ref com_palm_activitymanager_disable
 */

/* TODO: These public methods are left out of the generated documentation since they don't do anything.
 * - \ref com_palm_activitymanager_associate_app
 * - \ref com_palm_activitymanager_associate_service
 * - \ref com_palm_activitymanager_associate_process
 * - \ref com_palm_activitymanager_associate_network_flow
 * - \ref com_palm_activitymanager_dissociate_app
 * - \ref com_palm_activitymanager_dissociate_service
 * - \ref com_palm_activitymanager_dissociate_process
 * - \ref com_palm_activitymanager_dissociate_network_flow
 * - \ref com_palm_activitymanager_unmap_process
 */

const ActivityCategoryHandler::Method ActivityCategoryHandler::s_methods[] = {
	{ _T("create"), (Callback) &ActivityCategoryHandler::CreateActivity },
	{ _T("monitor"), (Callback) &ActivityCategoryHandler::MonitorActivity },
	{ _T("join"), (Callback) &ActivityCategoryHandler::JoinActivity },
	{ _T("release"), (Callback) &ActivityCategoryHandler::ReleaseActivity },
	{ _T("adopt"), (Callback) &ActivityCategoryHandler::AdoptActivity },
	{ _T("complete"), (Callback) &ActivityCategoryHandler::CompleteActivity },
	{ _T("schedule"), (Callback) &ActivityCategoryHandler::ScheduleActivity },
	{ _T("start"), (Callback) &ActivityCategoryHandler::StartActivity },
	{ _T("stop"), (Callback) &ActivityCategoryHandler::StopActivity },
	{ _T("cancel"), (Callback) &ActivityCategoryHandler::CancelActivity },
	{ _T("pause"), (Callback) &ActivityCategoryHandler::PauseActivity },
	{ _T("focus"), (Callback) &ActivityCategoryHandler::FocusActivity },
	{ _T("unfocus"), (Callback) &ActivityCategoryHandler::UnfocusActivity },
	{ _T("addFocus"), (Callback) &ActivityCategoryHandler::AddFocus },
	{ _T("list"), (Callback) &ActivityCategoryHandler::ListActivities },
	{ _T("getDetails"), (Callback) &ActivityCategoryHandler::GetActivityDetails },
	{ _T("associateApp"), (Callback) &ActivityCategoryHandler::AssociateApp },
	{ _T("associateService"), (Callback) &ActivityCategoryHandler::AssociateService },
	{ _T("associateProcess"), (Callback) &ActivityCategoryHandler::AssociateProcess },
	{ _T("associateNetworkFlow"), (Callback) &ActivityCategoryHandler::AssociateNetworkFlow },
	{ _T("dissociateApp"), (Callback) &ActivityCategoryHandler::DissociateApp },
	{ _T("dissociateService"), (Callback) &ActivityCategoryHandler::DissociateService },
	{ _T("dissociateProcess"), (Callback) &ActivityCategoryHandler::DissociateProcess },
	{ _T("dissociateNetworkFlow"), (Callback) &ActivityCategoryHandler::DissociateNetworkFlow },
	{ _T("mapProcess"), (Callback) &ActivityCategoryHandler::MapProcess },
	{ _T("unmapProcess"), (Callback) &ActivityCategoryHandler::UnmapProcess },
	{ _T("info"), (Callback) &ActivityCategoryHandler::Info },
	{ _T("enable"), (Callback) &ActivityCategoryHandler::Enable },
	{ _T("disable"), (Callback) &ActivityCategoryHandler::Disable },
	{ NULL, NULL }
};

MojLogger ActivityCategoryHandler::s_log(_T("activitymanager.activityhandler"));

ActivityCategoryHandler::ActivityCategoryHandler(
	boost::shared_ptr<PersistProxy> db,
	boost::shared_ptr<MojoJsonConverter> json,
	boost::shared_ptr<ActivityManager> am,
	boost::shared_ptr<MojoTriggerManager> triggerManager,
	boost::shared_ptr<PowerManager> powerManager,
	boost::shared_ptr<MasterResourceManager> resourceManager,
	boost::shared_ptr<ContainerManager> containerManager)
	: m_db(db)
	, m_json(json)
	, m_am(am)
	, m_triggerManager(triggerManager)
	, m_powerManager(powerManager)
	, m_resourceManager(resourceManager)
	, m_containerManager(containerManager)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Constructing"));
}

ActivityCategoryHandler::~ActivityCategoryHandler()
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Destructing"));
}

MojErr ActivityCategoryHandler::Init()
{
	MojLogTrace(s_log);
	MojLogNotice(s_log, _T("Initializing methods"));
	MojErr err;

#ifdef ACTIVITYMANAGER_USE_PUBLIC_BUS
	err = addMethods(s_methods, true);
#else
	err = addMethods(s_methods);
#endif

	MojErrCheck(err);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_create create

\e Public.

com.palm.activitymanager/create

<b>Creates a new Activity and returns its ID.</b>

You can create either a foreground or background Activity. A foreground Activity
is run as soon as its specified prerequisites are met. After a background
Activity's prerequisites are met, it is moved into a ready queue, and a limited
number are allowed to run depending on system resources. Foreground is the
default.

Activities can be scheduled to run at a specific time or when certain conditions
are met or events occur.

Each of your created and owned Activities must have a unique name. To replace
one of your existing Activities, set the "replace" flag to true. This cancels
the original Activity and replaces it with the new Activity.

To keep an Activity alive and receive status updates, the parent (and adopters,
if any) must set the "subscribe" flag to true.Activities with a callback and
"subscribe=false", the Activity must be adopted immediately after the callback
is invoked for the Activity to continue.

To indicate the Activity is fully-initialized and ready to launch, set the
`"start"` flag to `true`. Activities with a callback should be started when
created - the callback is invoked when the prerequisites have been met and, in
the case of a background, non-immediate Activity, it has been cleared to run.

<b>When requirements are not initially met</b>

If the creator of the Activity also specifies "subscribe":true, and detailed
events are enabled for that subscription, then the Activity Manager will
generate either an immediate "start" event if the requirements are met, or an
"update" event if the Activity is not yet ready to start due to missing
requirements, schedule, or trigger. This allows the creating Service to
determine if it should continue executing while waiting for the callback,
or exit to free memory if it may be awhile before the Activity is ready to run.

\subsection com_palm_activitymanager_create_syntax Syntax:
\code
{
    "activity": Activity object,
    "subscribe": boolean,
    "detailedEvents": boolean,
    "start": boolean,
    "replace": boolean
}
\endcode

\param activity Activity object. \e Required.
\param subscribe Subscribe to Activity flag.
\param detailedEvents Flag to have the Activity Manager generate "update" events
       when the state of one of this Activity's requirements changes.
\param start Start Activity immediately flag.
\param replace Cancel Activity and replace with Activity of same name flag.

\subsection com_palm_activitymanager_create_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean,
    "activityId": int
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.
\param activityId Activity ID.

\subsection com_palm_activitymanager_create_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/create '{ "activity": { "name": "basicactivity", "description": "Test create", "type": { "foreground": true } }, "start": true, "subscribe": true }'
\endcode

Example response for a succesful call, followed by an update event:
\code
{
    "activityId": 10813,
    "returnValue": true
}
{
    "activityId": 10813,
    "event": "start",
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 22,
    "errorText": "Activity specification not present",
    "returnValue": false
}
\endcode
*/
MojErr
ActivityCategoryHandler::CreateActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Create: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	MojObject spec;
	err = payload.getRequired(_T("activity"), spec);
	if (err) {
		err = msg->replyError(MojErrInvalidArg,
			"Activity specification not present");
		MojErrCheck(err);
		return MojErrNone;
	}

	/* First, parse and initialize the Activity.  No callouts to other
	 * Services will be generated until the Activity is scheduled to Start.
	 * It's very possible that the Create can fail (for various reasons)
	 * later, and that's fine... when the Activity is released it will be
	 * torn down. */
	boost::shared_ptr<Activity> act;

	try {
#ifdef ACTIVITYMANAGER_USE_PUBLIC_BUS
		MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
		if (!lunaMsg) {
			throw std::invalid_argument("Non-Luna Message passed in to "
				"Create");
		}

		Activity::BusType bus = lunaMsg->isPublic() ? Activity::PublicBus :
			Activity::PrivateBus;
#else
		Activity::BusType bus = Activity::PrivateBus;
#endif

		act = m_json->CreateActivity(spec, bus);

		/* If creator wasn't already set by the caller. */
		if (act->GetCreator().GetType() == BusNull) {
			act->SetCreator(MojoSubscription::GetBusId(msg));
		}
	} catch (const std::exception& except) {
		err = msg->replyError(MojErrNoMem, except.what());
		MojErrCheck(err);
		return MojErrNone;
	}


	/* Next, check to make sure we can move forward with creating the Activity.
	 * This essentially means:
	 * - The creator is going to subscribe or the Activity is going to start
	 *   now *and* has a callback (or else no one will know that it's
	 *   starting).
	 * - That no other Activity with the same name currently exists for this
	 *   creator, unless "replace" has been specified.
	 */
	bool subscribed = false;
	payload.get(_T("subscribe"), subscribed);

	bool start = false;
	payload.get(_T("start"), start);

	if (!subscribed && !(start && act->HasCallback())) {
		m_am->ReleaseActivity(act);

		err = msg->replyError(MojErrInvalidArg,
			"Created Activity must specify \"start\" and a Callback if not "
			"subscribed");
		MojErrCheck(err);
		return MojErrNone;
	}

	bool replace = false;
	payload.get(_T("replace"), replace);

	boost::shared_ptr<Activity> old;
	try {
		old = m_am->GetActivity(act->GetName(), act->GetCreator());
	} catch (...) {
		/* No Activity with that name/creator pair found. */
	}

	if (old && !replace) {
		MojLogWarning(s_log, _T("Create: Activity named \"%s\" created by "
			"\"%s\" already exists as [Activity %llu] and replace was not "
			"specified"), act->GetName().c_str(),
			act->GetCreator().GetString().c_str(), old->GetId());

		m_am->ReleaseActivity(act);
		err = msg->replyError(MojErrExists, _T("Activity with that name "
			"already exists"));
		MojErrCheck(err);
		return MojErrNone;
	}

	/* "Paaaaaaast the point of no return.... no backward glaaaanceeesss!!!!!"
	 *
	 * At this point, failure to create the Activity is an exceptional event.
	 * Persistence failures with MojoDB should be retried.  Ultimately this
	 * could end up with a hung Activity (if events aren't generated), or
	 * an Activity that ends up non-persisted.
	 *
	 * Stopping and starting the Activity Manager should return to a sane
	 * state, although clients with non-persistent Activities may be unhappy.
	 */

	if (old) {
		old->SetTerminateFlag(true);
		old->PlugAllSubscriptions();
		/* XXX protect against this failing */
		old->SendCommand(ActivityCancelCommand, true);
		if (old->IsNameRegistered()) {
			m_am->UnregisterActivityName(old);
		}
	}

	m_am->RegisterActivityId(act);
	m_am->RegisterActivityName(act);

	if (old) {
		EnsureReplaceCompletion(msg, payload, old, act);
		old->UnplugAllSubscriptions();
	} else {
		EnsureCompletion(msg, payload, PersistProxy::StoreCommandType,
			&ActivityCategoryHandler::FinishCreateActivity, act);
	}

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

void
ActivityCategoryHandler::FinishCreateActivity(
	MojRefCountedPtr<MojServiceMessage> msg, const MojObject& payload,
	boost::shared_ptr<Activity> act, bool succeeded)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Create finishing: Message from %s: "
		"[Activity %llu]: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		act->GetId(), succeeded ? "succeeded" : "failed");

	MojErr err = MojErrNone;

	bool start = false;
	payload.get(_T("start"), start);

	boost::shared_ptr<MojoSubscription> sub;
	bool subscribed = SubscribeActivity(msg.get(), payload, act, sub);
	if (subscribed) {
		act->SetParent(sub);
	}

	activityId_t id = act->GetId();

	MojObject reply;

	err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
	MojErrGoto(err, fail);

	err = reply.putInt(_T("activityId"), (MojInt64) id);
	MojErrGoto(err, fail);

	err = msg->reply(reply);
	MojErrGoto(err, fail);

	MojLogNotice(s_log, _T("Create complete: [Activity %llu] (\"%s\") created "
		"by %s"), act->GetId(), act->GetName().c_str(),
		MojoSubscription::GetSubscriberString(msg).c_str());

	if (start) {
		m_am->StartActivity(act);

		/* If an Activity isn't immediately running after being started,
		 * generate an update event to explain what the holdup is to any
		 * subscribers. */
		if (!act->IsRunning()) {
			act->BroadcastEvent(ActivityUpdateEvent);
		}
	}

	return;

fail:
	/* XXX Kill subscription */
	m_am->ReleaseActivity(act);

	ACTIVITY_SERVICEMETHODFINISH_END(msg);
}

void
ActivityCategoryHandler::FinishReplaceActivity(
	boost::shared_ptr<Activity> act, bool succeeded)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Replace finishing: Replace Activity [%llu]: %s"),
		act->GetId(), succeeded ? "succeeded" : "failed");

	/* Release the Activity's Persist Token.  The Activity that replaced it
	 * owns it now.  (This is really just a safety to make sure that any
	 * further actions on the Activity can't accidentally affect the
	 * persistence of the new Activity) */
	act->ClearPersistToken();
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_join join

\e Public.

com.palm.activitymanager/join

Subscribe to receive events from a particular Activity.

\subsection com_palm_activitymanager_join_syntax Syntax:
\code
{
    "activityId": int,
    "subscribe": true
}
\endcode

\param activityId The Activity to join to.
\param subscribe Must be true for this call to succeed.

\subsection com_palm_activitymanager_join_returns Returns:
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

\subsection com_palm_activitymanager_join_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/join '{ "activityId": 81, "subscribe": true }'
\endcode

Example response for a succesful call followed by a status update event:
\code
{
    "returnValue": true
}
{
    "activityId": 81,
    "event": "stop",
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 22,
    "errorText": "Join method calls must subscribe to the Activity",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::JoinActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Join: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = CheckSerial(msg, payload, act);
	MojErrCheck(err);

	boost::shared_ptr<MojoSubscription> sub;
	bool subscribed = SubscribeActivity(msg, payload, act, sub);
	if (!subscribed) {
		err = msg->replyError(MojErrInvalidArg,
			"Join method calls must subscribe to the Activity");
		MojErrCheck(err);
		return MojErrNone;
	}

	MojObject reply;

	err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);

	err = msg->reply(reply);
	MojErrCheck(err);

	MojLogNotice(s_log, _T("Join: %s subscribed to [Activity %llu]"),
		MojoSubscription::GetSubscriberString(msg).c_str(), act->GetId());

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_monitor monitor

\e Public.

com.palm.activitymanager/monitor

Given an activity ID, returns the current activity state. If the caller chooses
to subscribe, additional Activity status updates are returned as they occur.

\subsection com_palm_activitymanager_monitor_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "subscribe": boolean,
    "detailedEvents": boolean
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param subscribe Activity subscription flag. \e Required.
\param detailedEvents Flag to have the Activity Manager generate "update" events
       when the state of one of this Activity's requirements changes.
       \e Required.

\subsection com_palm_activitymanager_monitor_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean,
    "state": string
}

\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.
\param state Activity state.

\subsection com_palm_activitymanager_monitor_examples Examples:
\code
luna-send -i -f  luna://com.palm.activitymanager/monitor '{ "activityId": 71, "subscribe": false, "detailedEvents": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "state": "running"
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "Error retrieving activityId of Activity to operate on",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::MonitorActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Monitor: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = CheckSerial(msg, payload, act);
	MojErrCheck(err);

	boost::shared_ptr<MojoSubscription> sub;
	bool subscribed = SubscribeActivity(msg, payload, act, sub);

	MojObject reply;

	err = reply.putString(_T("state"), act->GetStateString().c_str());
	MojErrCheck(err);

	err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);

	err = msg->reply(reply);
	MojErrCheck(err);

	if (subscribed) {
		MojLogNotice(s_log, _T("Monitor: %s subscribed to [Activity %llu]"),
			MojoSubscription::GetSubscriberString(msg).c_str(), act->GetId());
	} else {
		MojLogNotice(s_log, _T("Monitor: Returned [Activity %llu] state to "
			"%s"), act->GetId(),
			MojoSubscription::GetSubscriberString(msg).c_str());
	}

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_release release

\e Public.

com.palm.activitymanager/release

Allows a parent to free an Activity and notify other subscribers. The Activity
is cancelled unless one of its non-parent subscribers adopts it and becomes the
new parent. This has to happen in the timeout specified. If no timeout is
specified, the Activity is cancelled immediately. For a completely safe
transfer, a subscribing app or service, prior to the release, should already
have called adopt, indicating its willingness to take over as the parent.

\subsection com_palm_activitymanager_release_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "timeout": int
}
\endcode

\param activityId Activity ID. Either this or "activityName" is required.
\param activityName Activity name. Either this or "activityId" is required.
\param timeout Time to wait, in seconds, for Activity to be adopted after release.

\subsection com_palm_activitymanager_release_returns Returns:
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

\subsection com_palm_activitymanager_release_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/release '{ "activityId": 2, "timeout": 30 }'
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
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::ReleaseActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Release: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = act->Release(MojoSubscription::GetBusId(msg));
	if (err) {
		MojLogNotice(s_log, _T("Release: %s failed to release [Activity %llu]"),
			MojoSubscription::GetSubscriberString(msg).c_str(), act->GetId());
	} else {
		MojLogNotice(s_log, _T("Release: [Activity %llu] successfully released "
			"by %s"), act->GetId(),
			MojoSubscription::GetSubscriberString(msg).c_str());
	}

	MojErrCheck(err);

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_adopt adopt

\e Public.

com.palm.activitymanager/adopt

Registers an app's or service's willingness to take over as the Activity's
parent.

If your app can wait for an unavailable Activity (not released) to become
available, then set the "wait" flag to true. If it is, and the Activity is valid
(exists and is not exiting), the call should succeed. If it cannot wait, and the
Activity is valid but cannot be adopted, then the call fails. The adopted return
flag indicates a successful or failed adoption.

If not immediately adopted and waiting is requested, the "orphan" event informs
the adopter that they are the new Activity parent.

An example where adoption makes sense is an app that allocates a separate
Activity for a sync, and passes that Activity to a service to use. The service
should adopt the Activity and be ready to take over in the event the app exits
before the service is done syncing. Otherwise, it receives a "cancel" event and
should exit immediately. The service should wait until the adopt (or monitor)
call returns successfully before beginning Activity work. If adopt or monitor
fails, it indicates the caller has quit or closed the Activity and the request
should not be processed. The Service should continue to process incoming events
on their subscription to the Activity.

If the service did not call adopt to indicate to the Activity Manager its
willingness to take over as the parent, it should be prepared to stop work for
the Activity and unsubscribe if it receives a "cancel" event. Otherwise, if it
receives the "orphan" event indicating the parent has unsubscribed and the
service is now the parent, then it should use complete when finished to inform
the Activity Manager before unsubscribing.

\subsection com_palm_activitymanager_adopt_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "wait": boolean,
    "subscribe": boolean,
    "detailedEvents": boolean
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param wait Wait for Activity to be released flag. \e Required.
\param subscribe Flag to subscribe to Activity and receive Activity events.
       \e Required.
\param detailedEvents Flag to have the Activity Manager generate "update" events
       when the state of an Activity's requirement changes. \e Required.

\subsection com_palm_activitymanager_adopt_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean,
    "adopted": boolean
}
\endcode


\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.
\param True if the Activity was adopted.

\subsection com_palm_activitymanager_adopt_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/adopt '{ "activityId": 876, "wait": true, "subscribe": true, "detailedEvents" : false }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "adopted": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::AdoptActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Adopt: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;
	bool wait = false;
	bool adopted = false;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = CheckSerial(msg, payload, act);
	MojErrCheck(err);

	boost::shared_ptr<MojoSubscription> sub;
	bool subscribed = SubscribeActivity(msg, payload, act, sub);

	if (!subscribed) {
		err = msg->replyError(MojErrInvalidArg,
			"AdoptActivity requires subscription");
		MojErrCheck(err);
		return MojErrNone;
	}

	payload.get(_T("wait"), wait);

	err = act->Adopt(sub, wait, &adopted);
	if (err) {
		MojLogNotice(s_log, _T("Adopt: %s failed to adopt [Activity %llu]"),
			sub->GetSubscriber().GetString().c_str(), act->GetId());
	} else if (adopted) {
		MojLogNotice(s_log, _T("Adopt: [Activity %llu] adopted by %s"),
			act->GetId(), sub->GetSubscriber().GetString().c_str());
	} else {
		MojLogNotice(s_log, _T("Adopt: %s queued to adopt [Activity %llu]"),
			sub->GetSubscriber().GetString().c_str(), act->GetId());
	}

	MojErrCheck(err);

	MojObject reply;

	err = reply.putBool(_T("adopted"), adopted);
	MojErrCheck(err);

	err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);

	err = msg->reply(reply);
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_complete complete

\e Public.

com.palm.activitymanager/complete

An Activity's parent can use this method to end the Activity and optionally
restart it with new attributes. If there are other subscribers, they are sent a
"complete" event.

If restart is requested, the Activity is optionally updated with any new
Callback, Schedule, Requirements, or Trigger data and returned to the queued
state. Specifying false for any of those properties removes the property
completely. For any properties not specified, current properties are used.

If the Activity is persistent (specified with the persist flag in the Type
object), the db8 database is updated before the call returns.

\subsection com_palm_activitymanager_complete_syntax Syntax:
\code
{
    "activityId": int
    "activityName": string,
    "restart": boolean,
    "callback": false or Callback,
    "schedule": false or Schedule,
    "requirements": false or Requirements,
    "trigger": false or Triggers,
    "metadata: false or any object
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param restart Restart Activity flag. Default is false.
\param callback Callback to use if Activity is restarted.
\param schedule Schedule to use if Activity is restarted.
\param requirements Prerequisites to use if Activity is restarted.
\param trigger Trigger to use if Activity is restarted.
\param metadata Meta data.

\subsection com_palm_activitymanager_complete_returns Returns:
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

\subsection com_palm_activitymanager_complete_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/complete '{ "activityId": 876 }'
\endcode

Example response for a succesful call:
\code
{
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": true
}
\endcode
*/

MojErr
ActivityCategoryHandler::CompleteActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Complete: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = CheckSerial(msg, payload, act);
	MojErrCheck(err);

	/* XXX Only from private bus - for testing only */
	bool force = false;
	payload.get(_T("force"), force);

	bool restart = false;
	payload.get(_T("restart"), restart);
	if (restart) {
		/* XXX Technically, if complete fails, the Activity would be updated
		 * anyways.  Which leads to a potentially odd condition.  Add a
		 * "CheckComplete" command to check to see if the command can succeed?
		 */
		m_json->UpdateActivity(act, payload);
		act->SetRestartFlag(true);
	} else {
		act->SetTerminateFlag(true);
	}

	act->PlugAllSubscriptions();

	/* XXX Catch and unplug */
	err = act->Complete(MojoSubscription::GetBusId(msg), force);
	if (err) {
		MojLogNotice(s_log, _T("Complete: %s failed to complete "
			"[Activity %llu]"),
			MojoSubscription::GetSubscriberString(msg).c_str(), act->GetId());

		/* XXX Remove these once CheckComplete is in place */
		act->SetTerminateFlag(false);
		act->UnplugAllSubscriptions();
	} else {
		MojLogNotice(s_log, _T("Complete: %s completing [Activity %llu]"),
			MojoSubscription::GetSubscriberString(msg).c_str(), act->GetId());
	}

	MojErrCheck(err);

	EnsureCompletion(msg, payload, restart ? PersistProxy::StoreCommandType :
		PersistProxy::DeleteCommandType,
		&ActivityCategoryHandler::FinishCompleteActivity, act);

	act->UnplugAllSubscriptions();

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

void
ActivityCategoryHandler::FinishCompleteActivity(
	MojRefCountedPtr<MojServiceMessage> msg, const MojObject& payload,
	boost::shared_ptr<Activity> act, bool succeeded)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Complete finishing: Message from %s: "
		"[Activity %llu]: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		act->GetId(), succeeded ? "succeeded" : "failed");

	bool restart = false;
	payload.get(_T("restart"), restart);

	if (!restart && act->IsNameRegistered()) {
		m_am->UnregisterActivityName(act);
	}

	MojErr err = msg->replySuccess();
	if (err)
		MojLogError(s_log, _T("Failed to generate reply to Complete request"));

	MojLogNotice(s_log, _T("Complete: %s completed [Activity %llu]"),
		MojoSubscription::GetSubscriberString(msg).c_str(), act->GetId());

	ACTIVITY_SERVICEMETHODFINISH_END(msg);
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_schedule schedule

\e Public.

com.palm.activitymanager/schedule

Set a schedule for an Activity.

\subsection com_palm_activitymanager_schedule_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "schedule": { object }
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param schedule A Schedule object.

\subsection com_palm_activitymanager_schedule_returns Returns:
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

\subsection com_palm_activitymanager_schedule_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.activitymanager/schedule '{ "activityId": 28, "schedule": { "interval": "2d", "local": true } }'
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
    "errorCode": 2,
    "errorText": "Error retrieving activityId of Activity to operate on",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::ScheduleActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Schedule: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = m_am->StartActivity(act);
	if (err) {
		MojLogNotice(s_log, _T("Schedule: Failed to schedule [Activity %llu]"),
			act->GetId());
	} else {
		MojLogNotice(s_log, _T("Schedule: [Activity %llu] scheduled"),
			act->GetId());
	}

	MojErrCheck(err);

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_start start

\e Public.

com.palm.activitymanager/start

Attempts to start the specified Activity, either moving it from the "init" state
to be eligible to run, or resuming it if it is currently paused. This sends
"start" events to any subscribed listeners once the Activity is cleared to
begin. If the "force" parameter is present (and true), other Activities could be
cancelled to free resources the Activity needs to run.

\subsection com_palm_activitymanager_start_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "force": boolean
}
\endcode

\param activityId Activity ID. Either this or "activityName" is required.
\param activityName Activity name. Either this or "activityId" is required.
\param force Force the Activity to run flag. If "true", other Activities could
             be cancelled to free resources the Activity needs to run.

\subsection com_palm_activitymanager_start_returns Returns:
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

\subsection com_palm_activitymanager_start_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/start '{ "activityId": 2, "force": true }'
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
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::StartActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Start: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = m_am->StartActivity(act);
	if (err) {
		MojLogNotice(s_log, _T("Start: Failed to start [Activity %llu]"),
			act->GetId());
	} else {
		MojLogNotice(s_log, _T("Start: [Activity %llu] started"), act->GetId());
	}

	MojErrCheck(err);

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_stop stop

\e Public.

com.palm.activitymanager/stop

Stops an Activity and sends a "stop" event to all Activity subscribers. This
succeeds unless the Activity is already cancelled.

This is different from the cancel method in that more time is allowed for the
Activity to clean up. For example, this might matter for an email sync service
that needs more time to finish downloading a large email attachment. On a
"cancel", it should immediately abort the download, clean up, and exit. On a
"stop," it should finish downloading the attachment in some reasonable amount of
time--say, 10-15 seconds.

\subsection com_palm_activitymanager_stop_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string
}
\endcode

\param activityId Activity ID. Either this or "activityName" is required.
\param activityName Activity name. Either this or "activityId" is required.

\subsection com_palm_activitymanager_stop_returns Returns:
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

\subsection com_palm_activitymanager_stop_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/stop '{ "activityId": 876 }'
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
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::StopActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Stop: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	act->PlugAllSubscriptions();

	act->SetTerminateFlag(true);

	err = m_am->StopActivity(act);
	if (err) {
		MojLogNotice(s_log, _T("Stop: Failed to stop [Activity %llu]"),
			act->GetId());
		act->UnplugAllSubscriptions();
	} else {
		MojLogNotice(s_log, _T("Stop: [Activity %llu] stopping"),
			act->GetId());
	}

	MojErrCheck(err);

	EnsureCompletion(msg, payload, PersistProxy::DeleteCommandType,
		&ActivityCategoryHandler::FinishStopActivity, act);

	act->UnplugAllSubscriptions();

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

void
ActivityCategoryHandler::FinishStopActivity(
	MojRefCountedPtr<MojServiceMessage> msg, const MojObject& payload,
	boost::shared_ptr<Activity> act, bool succeeded)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Stop finishing: Message from %s: [Activity %llu]:"
		" %s"), MojoSubscription::GetSubscriberString(msg).c_str(),
		act->GetId(), succeeded ? "succeeded" : "failed");

	if (act->IsNameRegistered()) {
		m_am->UnregisterActivityName(act);
	}

	MojErr err = msg->replySuccess();
	if (err)
		MojLogError(s_log, _T("Failed to generate reply to Stop request"));

	MojLogNotice(s_log, _T("Stop: [Activity %llu] stopped"), act->GetId());

	ACTIVITY_SERVICEMETHODFINISH_END(msg);
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_cancel cancel

\e Public.

com.palm.activitymanager/cancel

Terminates the specified Activity and sends a "cancel" event to all subscribers.
This call should succeed if the Activity exists and is not already exiting.

This is different from the stop method in that the Activity should take little
or no time to clean up. For example, this might matter for an email sync service
that needs more time to finish downloading a large email attachment. On a
"cancel", it should immediately abort the download, clean up, and exit. On a
"stop", it should finish downloading the attachment in some reasonable amount of
time, say, 10-15 seconds. Note, however, that specific time limits are not
currently enforced, though this could change.

\subsection com_palm_activitymanager_cancel_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string
}
\endcode

\param activityId Activity ID. Either this or "activityName" is required.
\param activityName Activity name. Either this or "activityId" is required.

\subsection com_palm_activitymanager_cancel_returns Returns:
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

\subsection com_palm_activitymanager_cancel_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/cancel '{ "activityId": 123 }'
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
    "errorCode": 2,
    "errorText": "Activity ID or name not present in request",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::CancelActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Cancel: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	act->PlugAllSubscriptions();

	act->SetTerminateFlag(true);

	err = m_am->CancelActivity(act);
	if (err) {
		MojLogNotice(s_log, _T("Cancel: Failed to cancel [Activity %llu]"),
			act->GetId());
		act->UnplugAllSubscriptions();
	} else {
		MojLogNotice(s_log, _T("Cancel: [Activity %llu] cancelling"),
			act->GetId());
	}

	MojErrCheck(err);

	EnsureCompletion(msg, payload, PersistProxy::DeleteCommandType,
		&ActivityCategoryHandler::FinishCancelActivity, act);

	act->UnplugAllSubscriptions();

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

void
ActivityCategoryHandler::FinishCancelActivity(
	MojRefCountedPtr<MojServiceMessage> msg, const MojObject& payload,
	boost::shared_ptr<Activity> act, bool succeeded)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Cancel finishing: Message from %s: [Activity %llu]"
		": %s"), MojoSubscription::GetSubscriberString(msg).c_str(),
		act->GetId(), succeeded ? "succeeded" : "failed");

	if (act->IsNameRegistered()) {
		m_am->UnregisterActivityName(act);
	}

	MojErr err = msg->replySuccess();
	if (err)
		MojLogError(s_log, _T("Failed to generate reply to Cancel request"));

	MojLogNotice(s_log, _T("Cancel: [Activity %llu] cancelled"),
		act->GetId());

	ACTIVITY_SERVICEMETHODFINISH_END(msg);
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_pause pause

\e Public.

com.palm.activitymanager/pause

Requests that the specified Activity be placed into the "paused" state. This
should succeed if the Activity is currently running or paused, but fail if the
Activity has ended (and is waiting for its subscribers to unsubscribe before
cleaning up) or has been cancelled.

\subsection com_palm_activitymanager_pause_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.

\subsection com_palm_activitymanager_pause_returns Returns:
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

\subsection com_palm_activitymanager_pause_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/pause '{ "activityId": 81 }'
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
    "errorCode": 2,
    "errorText": "Error retrieving activityId of Activity to operate on",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::PauseActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Pause: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = m_am->PauseActivity(act);
	if (err) {
		MojLogNotice(s_log, _T("Pause: Failed to pause [Activity %llu]"),
			act->GetId());
	} else {
		MojLogNotice(s_log, _T("Pause: [Activity %llu] pausing"), act->GetId());
	}

	MojErrCheck(err);

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_focus focus

\e Public.

com.palm.activitymanager/focus

Requests that focus be removed from all currently focused Activities, and
granted to the specified Activity. The requester must be privileged or this call
will fail.

Appropriate "focused" and "unfocused" events will be generated to subscribers of
Activities which gain or lose focus.

\subsection com_palm_activitymanager_focus_syntax Syntax:
\code
{
    "activityId": int
}
\endcode

\param activityId ID of the activity to grant focus to.

\subsection com_palm_activitymanager_focus_returns Returns:
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

\subsection com_palm_activitymanager_focus_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/focus '{ "activityId": 81 }'
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
    "errorCode": 2,
    "errorText": "Error retrieving activityId of Activity to operate on",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::FocusActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Focus: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = m_am->FocusActivity(act);
	if (err) {
		MojLogNotice(s_log, _T("Focus: Failed to focus [Activity %llu]"),
			act->GetId());
		err = msg->replyError(err, "Failed to focus Activity");
		MojErrCheck(err);
		return MojErrNone;
	} else {
		MojLogNotice(s_log, _T("Focus: [Activity %llu] focused"), act->GetId());
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_unfocus unfocus

\e Public.

com.palm.activitymanager/unfocus

Removes focus from the specified Activity.

\subsection com_palm_activitymanager_unfocus_syntax Syntax:
\code
{
    "activityId": int
}
\endcode

\param activityId ID of the activity to remove focus from.

\subsection com_palm_activitymanager_unfocus_returns Returns:
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

\subsection com_palm_activitymanager_unfocus_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/unfocus '{ "activityId": 81 }'
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
    "errorCode": 2,
    "errorText": "Error retrieving activityId of Activity to operate on",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::UnfocusActivity(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Unfocus: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	err = m_am->UnfocusActivity(act);
	if (err) {
		MojLogNotice(s_log, _T("Unfocus: Failed to unfocus [Activity %llu]"),
			act->GetId());
		err = msg->replyError(err, "Failed to unfocus Activity");
		MojErrCheck(err);
		return MojErrNone;
	} else {
		MojLogNotice(s_log, _T("Unfocus: [Activity %llu] unfocused"),
			act->GetId());
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_addfocus addfocus

\e Public.

com.palm.activitymanager/addfocus

Adds focus to the target Activity, if the source Activity has focus. If
successful, "focus" events will be generated to any subscribers of the target
Activity.

\subsection com_palm_activitymanager_addfocus_syntax Syntax:
\code
{
    "activityId": int,
    "targetActivityId": int
}
\endcode

\param activityId Activity that has focus.
\param targetActivityId Target activity to which to add focus.

\subsection com_palm_activitymanager_addfocus_returns Returns:
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

\subsection com_palm_activitymanager_addfocus_examples Examples:
\code
luna-send -i -f  luna://com.palm.activitymanager/addFocus '{ "activityId": 81, "targetActivityId": 70 }'
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
    "errorText": "Failed to add focus",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::AddFocus(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> sourceActivity;

	err = LookupActivity(msg, payload, sourceActivity);
	MojErrCheck(err);

	MojUInt64 targetId;
	bool found = false;
	err = payload.get(_T("targetActivityId"), targetId, found);
	MojErrCheck(err);

	if (!found) {
		err = msg->replyError(MojErrInvalidArg, "'targetActivityId' must be "
			"specified");
		MojErrCheck(err);
		return MojErrNone;
	}

	boost::shared_ptr<Activity> targetActivity;
	try {
		targetActivity = m_am->GetActivity((activityId_t)targetId);
	} catch (...) {
		err = msg->replyError(MojErrNotFound, "Target Activity not found");
		MojErrCheck(err);
		return MojErrNone;
	}

	err = m_am->AddFocus(sourceActivity, targetActivity);
	if (err) {
		MojLogNotice(s_log, _T("AddFocus: Failed to add focus from "
			"[Activity %llu] to [Activity %llu]"), sourceActivity->GetId(),
			targetActivity->GetId());
		err = msg->replyError(err, "Failed to add focus");
		MojErrCheck(err);
		return MojErrNone;
	} else {
		MojLogNotice(s_log, _T("AddFocus: Successfully added focus from "
			"[Activity %llu] to [Activity %llu]"), sourceActivity->GetId(),
			targetActivity->GetId());
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_list list

\e Public.

com.palm.activitymanager/list

List activities that are in running or waiting state.

\subsection com_palm_activitymanager_list_syntax Syntax:
\code
{
    "details": boolean,
    "subscribers": boolean,
    "current": boolean,
    "internal": boolean
}
\endcode

\param details Set to true to return full details for each Activity.
\param subscribers Set to true to return current parent, queued adopters, and
                   subscribers for each Activity
\param current Set to true to return the *current* state of the prerequisites
               for the Activity, as opposed to the desired states
\param internal Set to true to Include internal state information for debugging.


\subsection com_palm_activitymanager_list_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "activities": [ object array ],
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param activities Array with Activity objects.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_list_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.activitymanager/list '{ "details": true }'
\endcode

Example response for a succesful call:
\code
{
    "activities": [
        {
            "activityId": 2,
            "callback": {
                "method": "palm:\/\/com.palm.service.backup\/registerPubSub",
                "params": {
                }
            },
            "creator": {
                "serviceId": "com.palm.service.backup"
            },
            "description": "Registers the Backup Service with the PubSub Service, once the PubSub Service has a session.",
            "focused": false,
            "name": "RegisterPubSub",
            "state": "waiting",
            "trigger": {
                "method": "palm:\/\/com.palm.pubsubservice\/subscribeConnStatus",
                "params": {
                    "subscribe": true
                },
                "where": [
                    {
                        "op": "=",
                        "prop": "session",
                        "val": true
                    }
                ]
            },
            "type": {
                "background": true,
                "bus": "private"
            }
        },
        ...
        {
            "activityId": 223,
            "callback": {
                "method": "palm:\/\/com.palm.smtp\/syncOutbox",
                "params": {
                    "accountId": "++IAHN1DN8Snvorv",
                    "folderId": "++IAHN1nZl4k1OtL"
                }
            },
            "creator": {
                "serviceId": "com.palm.smtp"
            },
            "description": "Watches SMTP outbox for new emails",
            "focused": false,
            "metadata": {
                "accountId": "++IAHN1DN8Snvorv",
                "folderId": "++IAHN1nZl4k1OtL"
            },
            "name": "SMTP Watch\/accountId=\"\"++IAHN1DN8Snvorv\"\"",
            "requirements": {
                "internet": true
            },
            "state": "waiting",
            "trigger": {
                "key": "fired",
                "method": "palm:\/\/com.palm.db\/watch",
                "params": {
                    "query": {
                        "from": "com.palm.email:1",
                        "where": [
                            {
                                "_id": "454",
                                "op": "=",
                                "prop": "folderId",
                                "val": "++IAHN1nZl4k1OtL"
                            }
                        ]
                    }
                }
            },
            "type": {
                "bus": "private",
                "explicit": true,
                "immediate": true,
                "persist": true,
                "priority": "low"
            }
        }
    ],
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": -1989,
    "errorText": "json: unexpected char at 1:14",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::ListActivities(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("List: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	bool details = false;
	payload.get(_T("details"), details);

	bool subscribers = false;
	payload.get(_T("subscribers"), subscribers);

	bool current = false;
	payload.get(_T("current"), current);

	bool internal = false;
	payload.get(_T("internal"), internal);

	unsigned outputFlags = 0;
	if (details) outputFlags |= ACTIVITY_JSON_DETAIL;
	if (subscribers) outputFlags |= ACTIVITY_JSON_SUBSCRIBERS;
	if (current) outputFlags |= ACTIVITY_JSON_CURRENT;
	if (internal) outputFlags |= ACTIVITY_JSON_INTERNAL;

	MojErr err = MojErrNone;
	const ActivityManager::ActivityVec activities = m_am->GetActivities();

	MojObject activityArray(MojObject::TypeArray);
	for (ActivityManager::ActivityVec::const_iterator iter = activities.begin();
		iter != activities.end(); ++iter)
	{
		MojObject activity;

		err = (*iter)->ToJson(activity, outputFlags);
		MojErrCheck(err);

		err = activityArray.push(activity);
		MojErrCheck(err);
	}

	MojObject reply;
	err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);

	err = reply.put(_T("activities"), activityArray);
	MojErrCheck(err);

	err = msg->reply(reply);
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_get_details getDetails

\e Public.

com.palm.activitymanager/getDetails

Get details of an activity.

\subsection com_palm_activitymanager_get_details_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "current": boolean,
    "internal": boolean
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param current Set to true to return the *current* state of the prerequisites
               for the Activity, as opposed to the desired states
\param internal Set to true to Include internal state information for debugging.

\subsection com_palm_activitymanager_get_details_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "activity": { object },
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param activity The activity object.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_get_details_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/getDetails '{ "activityId": 221, "current": true }'
\endcode

Example response for a succesful call:
\code
{
    "activity": {
        "activityId": 221,
        "adopters": [
        ],
        "callback": <NULL>,
        "creator": {
            "serviceId": "com.palm.smtp"
        },
        "description": "Watches SMTP config on account \"++IAHN1DN8Snvorv\"",
        "focused": false,
        "metadata": {
            "accountId": "++IAHN1DN8Snvorv"
        },
        "name": "SMTP Account Watch\/accountId=\"\"++IAHN1DN8Snvorv\"\"",
        "state": "waiting",
        "subscribers": [
        ],
        "trigger": false,
        "type": {
            "bus": "private",
            "explicit": true,
            "immediate": true,
            "persist": true,
            "priority": "low"
        }
    },
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "Activity name\/creator pair not found",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::GetActivityDetails(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Details: Message from %s: %s"),
		MojoSubscription::GetSubscriberString(msg).c_str(),
		MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	boost::shared_ptr<Activity> act;

	err = LookupActivity(msg, payload, act);
	MojErrCheck(err);

	unsigned flags = ACTIVITY_JSON_DETAIL | ACTIVITY_JSON_SUBSCRIBERS;

	bool current = false;
	payload.get(_T("current"), current);
	if (current) {
		flags |= ACTIVITY_JSON_CURRENT;
	}

	bool internal = false;
	payload.get(_T("internal"), internal);
	if (internal) {
		flags |= ACTIVITY_JSON_INTERNAL;
	}

	MojObject rep;
	err = act->ToJson(rep, flags);
	MojErrCheck(err);

	MojObject reply;

	err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);

	err = reply.put(_T("activity"), rep);
	MojErrCheck(err);

	err = msg->reply(reply);
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_associate_app associateApp

\e Public.

com.palm.activitymanager/associateApp



\subsection com_palm_activitymanager_associate_app_syntax Syntax:
\code
{
}
\endcode

\subsection com_palm_activitymanager_associate_app_returns Returns:
\code
{
    "returnValue": boolean
}
\endcode

\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_associate_app_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/associateApp '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::AssociateApp(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_dissociate_app dissociateApp

\e Public.

com.palm.activitymanager/dissociateApp



\subsection com_palm_activitymanager_dissociate_app_syntax Syntax:
\code
{
}
\endcode

\subsection com_palm_activitymanager_dissociate_app_returns Returns:
\code
{
    "returnValue": boolean
}
\endcode

\param returnValue Indicates if the call wdis succesful.

\subsection com_palm_activitymanager_dissociate_app_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/dissociateApp '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::DissociateApp(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_associate_service associateService

\e Public.

com.palm.activitymanager/associateService



\subsection com_palm_activitymanager_associate_service_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_associate_service_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_associate_service_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/associateService '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::AssociateService(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_dissociate_service dissociateService

\e Public.

com.palm.activitymanager/dissociateService



\subsection com_palm_activitymanager_dissociate_service_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_dissociate_service_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_dissociate_service_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/dissociateService '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::DissociateService(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_associate_process associateProcess

\e Public.

com.palm.activitymanager/associateProcess



\subsection com_palm_activitymanager_associate_process_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_associate_process_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_associate_process_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/associateProcess '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::AssociateProcess(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_dissociate_process dissociateProcess

\e Public.

com.palm.activitymanager/dissociateProcess



\subsection com_palm_activitymanager_dissociate_process_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_dissociate_process_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_dissociate_process_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/dissociateProcess '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::DissociateProcess(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_associate_network_flow associateNetworkFlow

\e Public.

com.palm.activitymanager/associateNetworkFlow



\subsection com_palm_activitymanager_associate_network_flow_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_associate_network_flow_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_associate_network_flow_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/associateNetworkFlow '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::AssociateNetworkFlow(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_dissociate_network_flow dissociateNetworkFlow

\e Public.

com.palm.activitymanager/dissociateNetworkFlow



\subsection com_palm_activitymanager_dissociate_network_flow_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_dissociate_network_flow_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_dissociate_network_flow_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/dissociateNetworkFlow '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::DissociateNetworkFlow(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_map_process mapProcess

\e Private.

com.palm.activitymanager/mapProcess

Map a process and bus entities into a container.

\subsection com_palm_activitymanager_map_process_syntax Syntax:
\code
{
    "pid": int,
    "name": string,
    "ids": [ object array ]
}
\endcode

\param pid Id of the mapped process.
\param name The name of a mapping container to map the process into.
\param ids Array of entities on the bus that should be mapped into a container
           with the process.

\subsection com_palm_activitymanager_map_process_returns Returns:
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

\subsection com_palm_activitymanager_map_process_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/mapProcess '{ "pid": 221, "name": "container", "ids": [ {"appId": 1}, {"serviceId": 2}, {"anonId": 3} ] }'
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
    "errorText": "Must specify \"ids\" array of entities on the bus that should be mapped into a container with the process",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::MapProcess(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();
	MojErr err;

	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("MapProcess: %s"),
		MojoObjectJson(payload).c_str());

	MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
	if (!lunaMsg) {
		throw std::invalid_argument("Non-Luna Message passed in to "
			"MapProcess");
	}

	if (lunaMsg->isPublic()) {
		err = msg->replyError(MojErrAccessDenied, _T("Process mapping "
			"requests must originate from private bus"));
		MojErrCheck(err);
		return MojErrNone;
	}

	if (!m_containerManager) {
		err = msg->replyError(MojErrNotImplemented, _T("Mapping not "
			"implemented for this target"));
		MojErrCheck(err);
		return MojErrNone;
	}

	/* XXX Get the Manager for the type, try to cast into container manager. */
	MojUInt64 pid;	
	bool found = false;
	err = payload.get(_T("pid"), pid, found);
	if (err || !found) {
		err = msg->replyError(MojErrInvalidArg, _T("Must specify a \"pid\" to "
			"map"));
		MojErrCheck(err);
		return MojErrNone;
	}

	MojString containerName;
	found = false;
	err = payload.get(_T("name"), containerName, found);
	if (err || !found) {
		err = msg->replyError(MojErrInvalidArg, _T("Must specify the \"name\" "
			"of a mapping container to map the process into"));
		MojErrCheck(err);
		return MojErrNone;
	}

	MojObject idsJson;
	found = payload.get(_T("ids"), idsJson);
	if (!found || (idsJson.type() != MojObject::TypeArray)) {
		err = msg->replyError(MojErrInvalidArg, _T("Must specify \"ids\" "
			"array of entities on the bus that should be mapped into a "
			"container with the process"));
		MojErrCheck(err);
		return MojErrNone;
	}

	ContainerManager::BusIdVec ids;
	ids.reserve(idsJson.size());

	std::transform(idsJson.arrayBegin(), idsJson.arrayEnd(),
		std::back_inserter(ids), MojoJsonConverter::ProcessBusId);

	m_containerManager->MapContainer(containerName.data(), ids, (pid_t)pid);

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_unmap_process unmapProcess

\e Public.

com.palm.activitymanager/unmapProcess



\subsection com_palm_activitymanager_unmap_process_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_unmap_process_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_unmap_process_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/unmapProcess '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
ActivityCategoryHandler::UnmapProcess(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err = MojErrNone;

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_info info

\e Public.

com.palm.activitymanager/info

Get activity manager related information:
\li Activity Manager state:  Run queues and leaked Activities.
\li List of Activities for which power is currently locked.
\li State of the Resource Manager(s).

\subsection com_palm_activitymanager_info_syntax Syntax:
\code
{
}
\endcode

\subsection com_palm_activitymanager_info_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/info '{ }'
\endcode

Example response for a succesful call:
\code
{
    "queues": [
        {
            "activities": [
                {
                    "activityId": 28,
                    "creator": {
                        "serviceId": "com.palm.service.contacts.linker"
                    },
                    "name": "linkerWatch"
                },
                ...
                {
                    "activityId": 10,
                    "creator": {
                        "serviceId": "com.palm.db"
                    },
                    "name": "mojodbspace"
                }
            ],
            "name": "scheduled"
        }
    ],
    "resourceManager": {
        "entities": [
            {
                "activities": [
                ],
                "appId": "1",
                "focused": false,
                "priority": "none"
            },
            ...
            {
                "activities": [
                ],
                "anonId": "3",
                "focused": false,
                "priority": "none"
            }
        ],
        "resources": {
            "cpu": {
                "containers": [
                    {
                        "entities": [
                            {
                                "focused": false,
                                "priority": "none",
                                "serviceId": "com.palm.accountservices"
                            }
                        ],
                        "focused": false,
                        "name": "com.palm.accountservices",
                        "path": "\/dev\/cgroups\/com.palm.accountservices",
                        "priority": "low"
                    },
                    ...
                    {
                        "entities": [
                            {
                                "focused": false,
                                "priority": "none",
                                "serviceId": "2"
                            },
                            {
                                "anonId": "3",
                                "focused": false,
                                "priority": "none"
                            },
                            {
                                "appId": "1",
                                "focused": false,
                                "priority": "none"
                            }
                        ],
                        "focused": false,
                        "name": "container",
                        "path": "\/dev\/cgroups\/container",
                        "priority": "low"
                    }
                ],
                "entityMap": [
                    {
                        "serviceId:com.palm.netroute": "com.palm.netroute"
                    },
                    {
                        "serviceId:com.palm.connectionmanager": "com.palm.netroute"
                    },
                    ...
                    {
                        "serviceId:com.palm.preferences": "com.palm.preferences"
                    }
                ]
            }
        }
    }
}
\endcode
*/

MojErr
ActivityCategoryHandler::Info(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	MojErr err;

	MojObject reply(MojObject::TypeObject);

	/* Get the Activity Manager state:  Run queues and leaked Activities */
	err = m_am->InfoToJson(reply);
	MojErrCheck(err);

	/* Get the list of Activities for which power is currently locked */
	err = m_powerManager->InfoToJson(reply);
	MojErrCheck(err);

	/* Get the state of the Resource Manager(s) */
	err = m_resourceManager->InfoToJson(reply);
	MojErrCheck(err);

	err = msg->reply(reply);
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_enable enable

\e Private.

com.palm.activitymanager/enable

Enable scheduling new Activities.

\subsection com_palm_activitymanager_enable_syntax Syntax:
\code
{
}
\endcode

\subsection com_palm_activitymanager_enable_returns Returns:
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

\subsection com_palm_activitymanager_enable_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/enable '{ }'
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
    "errorCode": 13,
    "errorText": "Only callers on the private bus are allowed to enable or disable Activity dispatch",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::Enable(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	if (IsPublicMessage(msg)) {
		MojErr err = msg->replyError(MojErrAccessDenied, "Only callers on the "
			"private bus are allowed to enable or disable Activity dispatch");
		MojErrCheck(err);
	} else {
		MojLogNotice(s_log, _T("Enabling scheduling new Activities"));
		m_am->Enable(ActivityManager::EXTERNAL_ENABLE);

		MojErr err = msg->replySuccess();
		MojErrCheck(err);
	}

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_disable disable

\e Private.

com.palm.activitymanager/disable

Disable scheduling new Activities.

\subsection com_palm_activitymanager_disable_syntax Syntax:
\code
{
}
\endcode

\subsection com_palm_activitymanager_disable_returns Returns:
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

\subsection com_palm_activitymanager_disable_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/disable '{ }'
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
    "errorCode": 13,
    "errorText": "Only callers on the private bus are allowed to enable or disable Activity dispatch",
    "returnValue": false
}
\endcode
*/

MojErr
ActivityCategoryHandler::Disable(MojServiceMessage *msg, MojObject& payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);

	if (IsPublicMessage(msg)) {
		MojErr err = msg->replyError(MojErrAccessDenied, "Only callers on the "
			"private bus are allowed to enable or disable Activity dispatch");
		MojErrCheck(err);
	} else {
		MojLogNotice(s_log, _T("Disabling scheduling new Activities"));
		m_am->Disable(ActivityManager::EXTERNAL_ENABLE);

		MojErr err = msg->replySuccess();
		MojErrCheck(err);
	}

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

MojErr
ActivityCategoryHandler::LookupActivity(MojServiceMessage *msg, MojObject& payload, boost::shared_ptr<Activity>& act)
{
	try {
		act = m_json->LookupActivity(payload, MojoSubscription::GetBusId(msg));
	} catch (const std::exception& except) {
		msg->replyError(MojErrNotFound, except.what());
		return MojErrNotFound;
	}

	return MojErrNone;
}

bool
ActivityCategoryHandler::IsPublicMessage(MojServiceMessage *msg) const
{
	MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
	if (!lunaMsg) {
		throw std::invalid_argument("Non-Luna Message");
	}

	return lunaMsg->isPublic();
}

bool
ActivityCategoryHandler::SubscribeActivity(MojServiceMessage *msg,
	const MojObject& payload, const boost::shared_ptr<Activity>& act,
	boost::shared_ptr<MojoSubscription>& sub)
{
	MojLogTrace(s_log);

	bool subscribe = false;
	payload.get(_T("subscribe"), subscribe);

	if (!subscribe)
		return false;

	bool detailedEvents = false;
	payload.get(_T("detailedEvents"), detailedEvents);

	sub = boost::make_shared<MojoSubscription>(act, detailedEvents, msg);
	sub->EnableSubscription();
	act->AddSubscription(sub);

	return true;
}

MojErr
ActivityCategoryHandler::CheckSerial(MojServiceMessage *msg,
	const MojObject& payload, const boost::shared_ptr<Activity>& act)
{
	MojErr err;
	bool found = false;

	MojUInt32 serial;

	err = payload.get(_T("serial"), serial, found);
	MojErrCheck(err);

	if (found) {
		if (!act->HasCallback()) {
			throw std::runtime_error("Attempt to use callback sequence serial "
				"number match on Activity that does not have a callback");
		}

		if (act->GetCallback()->GetSerial() != serial) {
			MojLogWarning(s_log, _T("[Activity %llu] %s attempting to "
				"interact with call chain with sequence serial number %u, "
				"but it is currently at %u"), act->GetId(),
				MojoSubscription::GetSubscriberString(msg).c_str(),
				serial, act->GetCallback()->GetSerial());
			throw std::runtime_error("Call sequence serial numbers do not "
				"match");
		}
	}

	return MojErrNone;
}

void
ActivityCategoryHandler::EnsureCompletion(MojServiceMessage *msg,
	MojObject& payload, PersistProxy::CommandType type,
	CompletionFunction func, boost::shared_ptr<Activity> act)
{
	MojLogTrace(s_log);

	if (act->IsPersistent()) {
		/* Ensure a Persist Token object has been allocated for the Activity.
		 * This should only be necessary on initial Create of the Activity. */
		if (!act->IsPersistTokenSet()) {
			act->SetPersistToken(m_db->CreateToken());
		}

		boost::shared_ptr<Completion> completion =
			boost::make_shared<MojoMsgCompletion<ActivityCategoryHandler> >
				(this, func, msg, payload, act);

		boost::shared_ptr<PersistCommand> cmd = m_db->PrepareCommand(
			type, act, completion);

		if (act->IsPersistCommandHooked()) {
			act->HookPersistCommand(cmd);
			act->GetHookedPersistCommand()->Append(cmd);
		} else {
			act->HookPersistCommand(cmd);
			cmd->Persist();
		}
	} else if (act->IsPersistCommandHooked()) {
		boost::shared_ptr<Completion> completion =
			boost::make_shared<MojoMsgCompletion<ActivityCategoryHandler> >
				(this, func, msg, payload, act);

		boost::shared_ptr<PersistCommand> cmd = m_db->PrepareNoopCommand(
			act, completion);

		act->HookPersistCommand(cmd);
		act->GetHookedPersistCommand()->Append(cmd);
	} else {
		((*this).*func)(msg, payload, act, true);
	}
}

/* XXX TEST ALL THE CASES HERE */
void
ActivityCategoryHandler::EnsureReplaceCompletion(MojServiceMessage *msg,
	MojObject& payload, boost::shared_ptr<Activity> oldActivity,
	boost::shared_ptr<Activity> newActivity)
{
	MojLogTrace(s_log);

	if (!oldActivity) {
		MojLogError(s_log, _T("Activity which new [Activity %llu] is to "
			"replace was not specified"), newActivity->GetId());
		throw std::runtime_error("Activity to be replaced must be specified");
	}

	if (newActivity->IsPersistTokenSet()) {
		MojLogError(s_log, _T("New [Activity %llu] already has persist token "
			"set while preparing it to replace existing [Activity %llu]"),
			newActivity->GetId(), oldActivity->GetId());
		throw std::runtime_error("New Activity already has persist token "
			"set while replacing existing Activity");
	}

	if (newActivity->IsPersistent()) {
		/* Activities can only get Tokens on Create (or Load at startup).  If
		 * it's missing it's because the Activity was never persistent or was
		 * cancelled */
		if (oldActivity->IsPersistTokenSet()) {
			newActivity->SetPersistToken(oldActivity->GetPersistToken());
		} else {
			newActivity->SetPersistToken(m_db->CreateToken());
		}

		boost::shared_ptr<Completion> newCompletion =
			boost::make_shared<MojoMsgCompletion<ActivityCategoryHandler> >
				(this, &ActivityCategoryHandler::FinishCreateActivity,
				msg, payload, newActivity);
		boost::shared_ptr<PersistCommand> newCmd = m_db->PrepareStoreCommand(
			newActivity, newCompletion);

		boost::shared_ptr<Completion> oldCompletion =
			boost::make_shared<MojoRefCompletion<ActivityCategoryHandler> >
				(this, &ActivityCategoryHandler::FinishReplaceActivity,
				oldActivity);
		boost::shared_ptr<PersistCommand> oldCmd = m_db->PrepareNoopCommand(
			oldActivity, oldCompletion);

		/* New command first (it's the blocking one), then old command */
		newActivity->HookPersistCommand(newCmd);
		newCmd->Append(oldCmd);

		if (oldActivity->IsPersistCommandHooked()) {
			oldActivity->HookPersistCommand(oldCmd);
			oldActivity->GetHookedPersistCommand()->Append(newCmd);
		} else {
			oldActivity->HookPersistCommand(oldCmd);
			newCmd->Persist();
		}
	} else if (oldActivity->IsPersistent()) {
		/* Prepare command to delete the old Activity */
		boost::shared_ptr<Completion> oldCompletion =
			boost::make_shared<MojoRefCompletion<ActivityCategoryHandler > >
				(this, &ActivityCategoryHandler::FinishReplaceActivity,
				oldActivity);
		boost::shared_ptr<PersistCommand> oldCmd = m_db->PrepareDeleteCommand(
			oldActivity, oldCompletion);

		/* Ensure there's a command attached to the new Activity in case
		 * someone tries to replace *it*.  That can't succeed until the
		 * original command is succesfully replaced.  This noop command
		 * will complete the replace. */
		boost::shared_ptr<Completion> newCompletion =
			boost::make_shared<MojoMsgCompletion<ActivityCategoryHandler> >
				(this, &ActivityCategoryHandler::FinishCreateActivity,
				msg, payload, newActivity);
		boost::shared_ptr<PersistCommand> newCmd = m_db->PrepareNoopCommand(
			newActivity, newCompletion);

		/* Old command first (it's the blocking one), then complete the
		 * new command - the create. */
		newActivity->HookPersistCommand(newCmd);
		oldCmd->Append(newCmd);

		/* Remember to hook the command *after* checking if there is already
		 * one hooked. */
		if (oldActivity->IsPersistCommandHooked()) {
			oldActivity->HookPersistCommand(oldCmd);
			oldActivity->GetHookedPersistCommand()->Append(oldCmd);
		} else {
			oldActivity->HookPersistCommand(oldCmd);
			oldCmd->Persist();
		}
	} else if (oldActivity->IsPersistCommandHooked()) {
		/* Activity B (not persistent) replaced Activity A (persistent),
		 * which is still being deleted. Now Activity C is trying to replace
		 * Activity B, but since A hasn't finished yet, C shouldn't finish
		 * either.  A crash might leave A existing while C existed
		 * externally.  Which would be naughty. */
		boost::shared_ptr<Completion> newCompletion =
			boost::make_shared<MojoMsgCompletion<ActivityCategoryHandler> >
				(this, &ActivityCategoryHandler::FinishCreateActivity,
				msg, payload, newActivity);
		boost::shared_ptr<PersistCommand> newCmd = m_db->PrepareNoopCommand(
			newActivity, newCompletion);

		boost::shared_ptr<Completion> oldCompletion =
			boost::make_shared<MojoRefCompletion<ActivityCategoryHandler> >
				(this, &ActivityCategoryHandler::FinishReplaceActivity,
					oldActivity);
		boost::shared_ptr<PersistCommand> oldCmd = m_db->PrepareNoopCommand(
			oldActivity, oldCompletion);

		/* Wait for whatever that command is ultimately waiting for */
		newActivity->HookPersistCommand(newCmd);
		newCmd->Append(oldCmd);
		oldActivity->HookPersistCommand(oldCmd);
		oldActivity->GetHookedPersistCommand()->Append(newCmd);
	} else {
		FinishCreateActivity(MojRefCountedPtr<MojServiceMessage>(msg),
			payload, newActivity, true);
		FinishReplaceActivity(oldActivity, true);
	}
}

