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

