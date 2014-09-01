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
#include "ActivityManager.h"
#include "PowerActivity.h"
#include "PowerManager.h"
#include "Trigger.h"
#include "Callback.h"
#include "Subscription.h"
#include "Schedule.h"
#include "ActivityJson.h"
#include "Requirement.h"
#include "ActivityAutoAssociation.h"
#include "Logging.h"
#include <stdexcept>
#include <functional>
#include <algorithm>

MojLogger Activity::s_log(_T("activitymanager.activity"));

Activity::Activity(activityId_t id, boost::weak_ptr<ActivityManager> am)
	: m_id(id)
	, m_persistent(false)
	, m_explicit(false)
	, m_immediate(false)
	, m_priority(ActivityBackgroundPriority)
	, m_useSimpleType(true) /* Background */
	, m_focused(false)
	, m_continuous(false)
	, m_userInitiated(false)
	, m_powerDebounce(false)
	, m_bus(PublicBus)
	, m_initialized(false)
	, m_scheduled(false)
	, m_ready(false)
	, m_running(false)
	, m_ending(false)
	, m_terminate(false)
	, m_restart(false)
	, m_requeue(false)
	, m_yielding(false)
	, m_released(true)
	, m_intCommand(ActivityNoCommand)
	, m_extCommand(ActivityNoCommand)
	, m_sentCommand(ActivityNoCommand)
	, m_am(am)
{
}

Activity::~Activity()
{
	LOG_AM_DEBUG("[Activity %llu] Cleaning up", m_id);
}

activityId_t Activity::GetId() const
{
	return m_id;
}

void Activity::SetName(const std::string& name)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);

	m_name = name;
}

const std::string& Activity::GetName() const
{
	return m_name;
}

bool Activity::IsNameRegistered() const
{
	return m_nameTableItem.is_linked();
}

void Activity::SetDescription(const std::string& description)
{
	m_description = description;
}

const std::string& Activity::GetDescription() const
{
	return m_description;
}

void Activity::SetCreator(const Subscriber& creator)
{
	m_creator = creator;
}

void Activity::SetCreator(const BusId& creator)
{
	m_creator = creator;
}

const BusId& Activity::GetCreator() const
{
	return m_creator;
}

void Activity::SetImmediate(bool immediate)
{
	m_immediate = immediate;
}

bool Activity::IsImmediate() const
{
	return m_immediate;
}

std::string Activity::GetStateString() const
{
	return std::string(ActivityStateNames[GetState()]);
}

void Activity::SendCommand(ActivityCommand_t command, bool internal)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] \"%s\" command received %s",
		m_id, ActivityCommandNames[command],
		internal ? " (internally generated)" : "");

	/* Cancel, Stop, and Complete are final.  Once you go there, you
	 * can't change your mind. */
	if (internal) {
		if ((m_intCommand == ActivityCancelCommand) || 
			(m_intCommand == ActivityStopCommand)) {
			throw std::runtime_error("Activity Manager has already generated "
				"a final command to this Activity");
		}
		m_intCommand = command;
	} else {
		if ((command == ActivityStartCommand) && !m_initialized &&
			!m_ending) {
			RequestScheduleActivity();
		}

		if ((m_extCommand == ActivityCancelCommand) ||
			(m_extCommand == ActivityStopCommand) ||
			(m_extCommand == ActivityCompleteCommand)) {
			throw std::runtime_error("The Activity Manager has already "
				"received a command to end the Activity");
		}

		m_extCommand = command;
	}

	/* If the Activity is already ending, don't do anything. */
	if (m_ending)
		return;

	ActivityCommand_t sendCommand = ComputeNextExternalCommand();
	if (sendCommand != m_sentCommand) {
		/* If the Activity has never started externally, only commands
		 * to start or end it are allowed. */
		if ((sendCommand == ActivityCancelCommand) ||
			(sendCommand == ActivityStopCommand) ||
			(sendCommand == ActivityCompleteCommand)) {
			BroadcastCommand(sendCommand);
			EndActivity();
		} else if (!m_running && (sendCommand == ActivityStartCommand)) {
			/* Activity has not yet started running, so only let it run if
			 * it has been scheduled to run, triggered, and its requirements
			 * are met. */
			if (IsRunnable() && !m_ready) {
				RequestRunActivity();
			}
		} else {
			/* Activity has already started running, so this would be
			 * a transition to or from Pause... just pass on the command */
			BroadcastCommand(sendCommand);
		}
	}
}

MojErr Activity::AddSubscription(boost::shared_ptr<Subscription> sub)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] %s subscribed",
		m_id, sub->GetSubscriber().GetString().c_str());

	m_subscriptions.insert(*sub);

	SubscriberSet::const_iterator exists = m_subscribers.find(
		sub->GetSubscriber());
	if (exists != m_subscribers.end()) {
		m_subscribers.insert(exists, sub->GetSubscriber());
	} else {
		m_subscribers.insert(sub->GetSubscriber());
		/* If the Activity isn't running, don't pass this information on
		 * to the Activity Manager.  It will query for the unique subscriber
		 * list immediately before it shifts the Activity to the running
		 * state. */
		if (m_running) {
			m_am.lock()->InformActivityGainedSubscriberId(shared_from_this(),
				sub->GetSubscriber().GetId());
		}
	}

	return MojErrNone;
}

MojErr Activity::RemoveSubscription(boost::shared_ptr<Subscription> sub)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] %s unsubscribed",
		m_id, sub->GetSubscriber().GetString().c_str());

	SubscriptionSet::iterator subscription = m_subscriptions.begin();

	for (; subscription != m_subscriptions.end(); ++subscription) {
		if (&(*sub) == &(*subscription))
			break;
	}

	if (subscription != m_subscriptions.end()) {
		m_subscriptions.erase(subscription);
	} else {
		LOG_AM_WARNING(MSGID_SUBSCRIBER_NOT_FOUND, 2, PMLOGKFV("activity","%llu",m_id),
			    PMLOGKS("subscriber",sub->GetSubscriber().GetString().c_str()),
			    "activity unable to find subscriber on the subscriptions list");
		return MojErrInvalidArg;
	}

	Subscriber& actor = sub->GetSubscriber();

	SubscriberSet::iterator subscriber = m_subscribers.begin();
	for (; subscriber != m_subscribers.end(); ++subscriber) {
		if (&actor == &(*subscriber))
			break;
	}

	if (subscriber != m_subscribers.end()) {
		m_subscribers.erase(subscriber);
	} else {
		LOG_AM_DEBUG("[Activity %llu] Unable to find %s's subscriber entry on the subscribers list",
			m_id,actor.GetString().c_str());
	}

	/* If all subscriptions by that subscriber are gone, now... */
	if (m_subscribers.find(actor) == m_subscribers.end()) {
		/* Only if running.  If not running, the Activity Manager will
		 * query the Activity for all unique Subscribers before
		 * running. */
		if (m_running) {
			m_am.lock()->InformActivityLostSubscriberId(shared_from_this(),
				actor.GetId());
		}
	}

	AdopterQueue::iterator adopter = m_adopters.begin();
	for (; adopter != m_adopters.end(); ++adopter) {
		if (sub == adopter->lock())
			break;
	}

	if (adopter != m_adopters.end()) {
		m_adopters.erase(adopter);
	}

	if (m_releasedParent.lock() == sub) {
		m_releasedParent.reset();
	}

	if (m_parent.lock() == sub) {
		m_parent.reset();

		if (m_adopters.empty()) {
			/* If there are no remaining subscribers, just let Abandoned clean
			 * up and call EndActivity. */
			if (!m_subscriptions.empty()) {
				Orphaned();
			}
		} else {
			m_parent = m_adopters.front().lock();
			m_adopters.pop_front();

			m_parent.lock()->QueueEvent(ActivityOrphanEvent);
		}
	}

	if (m_subscriptions.empty()) {
		Abandoned();
	}

	return MojErrNone;
}

bool Activity::IsSubscribed() const
{
	return !m_subscriptions.empty();
}

Activity::SubscriberIdVec Activity::GetUniqueSubscribers() const
{
	SubscriberIdVec subscribers;
	subscribers.reserve(m_subscribers.size());

	std::unique_copy(m_subscribers.begin(), m_subscribers.end(),
		std::back_inserter(subscribers));

	return subscribers;
}

MojErr Activity::BroadcastEvent(ActivityEvent_t event)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Broadcasting \"%s\" event", m_id,
		ActivityEventNames[event]);

	MojErr err = MojErrNone;
	
	for (SubscriptionSet::iterator iter = m_subscriptions.begin();
		iter != m_subscriptions.end(); ++iter) {
		MojErr sendErr = iter->QueueEvent(event);

		/* Catch the last error, if any */
		if (sendErr)
			err = sendErr;
	}

        if (err != MojErrNone)
            LOG_AM_DEBUG("[Activity %llu] catch the last error %d", err);

	return MojErrNone;
}

void Activity::PlugAllSubscriptions()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Plugging all subscriptions", m_id);

	for (SubscriptionSet::iterator iter = m_subscriptions.begin();
		iter != m_subscriptions.end(); ++iter) {
		iter->Plug();
	}
}

void Activity::UnplugAllSubscriptions()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Unplugging all subscriptions", m_id);

	for (SubscriptionSet::iterator iter = m_subscriptions.begin();
		iter != m_subscriptions.end(); ++iter) {
		iter->Unplug();
	}
}

void Activity::SetTrigger(boost::shared_ptr<Trigger> trigger)
{
	m_trigger = trigger;
}

boost::shared_ptr<Trigger> Activity::GetTrigger() const
{
	return m_trigger;
}

void Activity::ClearTrigger()
{
	m_trigger.reset();
}

bool Activity::HasTrigger() const
{
	return m_trigger;
}

void Activity::Triggered(boost::shared_ptr<Trigger> trigger)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Triggered", m_id);

	if (m_trigger == trigger) {
		if (!m_running && !m_ready && IsRunnable()) {
			RequestRunActivity();
		}
	}
}

bool Activity::IsTriggered() const
{
	if (m_trigger) {
		return m_trigger->IsTriggered(shared_from_this());
	} else {
		return true;
	}
}

void Activity::SetCallback(boost::shared_ptr<Callback> callback)
{
	m_callback = callback;
}

boost::shared_ptr<Callback> Activity::GetCallback()
{
	return m_callback;
}

bool Activity::HasCallback() const
{
	return m_callback;
}

void Activity::CallbackFailed(boost::shared_ptr<Callback> callback,
	Callback::FailureType failure)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);

	if (failure == Callback::TransientFailure) {
		LOG_AM_DEBUG("[Activity %llu] Callback experienced transient protocol failure... requeuing Activity",
			m_id);
		RequestRequeueActivity();
	} else {
		LOG_AM_DEBUG("[Activity %llu] Callback experienced permanent failure... cancelling Activity",
			m_id);
		SetTerminateFlag(true);
		SendCommand(ActivityCancelCommand, true);
		/* XXX de-persist here */
	}
}

void Activity::CallbackSucceeded(boost::shared_ptr<Callback> callback)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Callback succeeded", m_id);

	/* Nothing special to do here unless the Activity itself implements a
	 * callback timeout. */
}

void Activity::SetSchedule(boost::shared_ptr<Schedule> schedule)
{
	m_schedule = schedule;
}

void Activity::ClearSchedule()
{
	m_schedule.reset();
}

void Activity::Scheduled()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Scheduled", m_id);

	if (!m_running && !m_ready && IsRunnable()) {
		RequestRunActivity();
	}
}

bool Activity::IsScheduled() const
{
	if (m_schedule) {
		return m_schedule->IsScheduled();
	} else {
		return true;
	}
}

void Activity::AddRequirement(boost::shared_ptr<Requirement> requirement)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] setting [Requirement %s]", m_id,
		requirement->GetName().c_str());

	if (requirement->GetActivity() != shared_from_this()) {
		LOG_AM_ERROR(MSGID_ADD_REQ_OWNER_MISMATCH, 3,
			  PMLOGKS("Requirement",requirement->GetActivity()->GetName().c_str()),
			  PMLOGKFV("from_activity","%llu",requirement->GetActivity()->GetId()),
			  PMLOGKFV("to_activity","%llu",m_id), "");
		throw std::runtime_error("Requirement owner mismatch");
	}

	m_requirements[requirement->GetName()] = requirement;

	if (requirement->m_activityListItem.is_linked()) {
		LOG_AM_DEBUG("Found linked requirement adding [Requirement %s] to [Activity %llu]",
			requirement->GetName().c_str(), m_id);
		requirement->m_activityListItem.unlink();
	}

	if (requirement->IsMet()) {
		m_metRequirements.push_back(*requirement);
	} else {
		m_unmetRequirements.push_back(*requirement);

		if (!m_running && m_ready) {
			m_ready = false;
			m_am.lock()->InformActivityNotReady(shared_from_this());
		}
	}
}

void Activity::RemoveRequirement(boost::shared_ptr<Requirement> requirement)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] removing [Requirement %s]", m_id,
		requirement->GetName().c_str());

	if (requirement->GetActivity() != shared_from_this()) {
		LOG_AM_ERROR(MSGID_REMOVE_REQ_OWNER_MISMATCH, 3, PMLOGKFV("Requirement_owner","%llu",requirement->GetActivity()->GetId()),
			  PMLOGKS("Requirement",requirement->GetName().c_str()),
			  PMLOGKFV("Activity","%llu",m_id), "");
		throw std::runtime_error("Requirement owner mismatch");
	}

	if (!requirement->m_activityListItem.is_linked()) {
		LOG_AM_DEBUG("Found unlinked requirement removing [Requirement %s] from [Activity %llu]",
			requirement->GetName().c_str(), m_id);
	} else {
		requirement->m_activityListItem.unlink();
	}

	if (!m_running && !m_ready && IsRunnable()) {
		RequestRunActivity();
	}
}

void Activity::RemoveRequirement(const std::string& name)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] removing [Requirement %s] by name",
		m_id, name.c_str());

	RequirementMap::iterator found = m_requirements.find(name);
	if (found == m_requirements.end()) {
		LOG_AM_WARNING(MSGID_RM_REQ_NOT_FOUND , 2,
			PMLOGKFV("Activity","%llu",m_id),
			PMLOGKS("Requirement",name.c_str()),
			"not found while trying to remove by name");
		return;
	}

	if (found->second->m_activityListItem.is_linked()) {
		found->second->m_activityListItem.unlink();
	} else {
		LOG_AM_DEBUG("Found unlinked requirement removing [Requirement %s] from [Activity %llu] by name",
			found->second->GetName().c_str(), m_id);
	}

	m_requirements.erase(found);

	if (!m_running && !m_ready && IsRunnable()) {
		RequestRunActivity();
	}
}

bool Activity::IsRequirementSet(const std::string& name) const
{
	RequirementMap::const_iterator found = m_requirements.find(name);
	return (found != m_requirements.end());
}

bool Activity::HasRequirements() const
{
	return (!m_requirements.empty());
}

void Activity::RequirementMet(boost::shared_ptr<Requirement> requirement)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] [Requirement %s] met", m_id,
		requirement->GetName().c_str());

	if (requirement->GetActivity() != shared_from_this()) {
		LOG_AM_ERROR(MSGID_REQ_MET_OWNER_MISMATCH,3,
			PMLOGKS("Requirement",requirement->GetName().c_str()),
            PMLOGKFV("OwnedActivity","%llu",requirement->GetActivity()->GetId()),
			PMLOGKFV("CurrentActivity","%llu",m_id),
			"Activity id differ for met");
		throw std::runtime_error("Requirement owner mismatch");
	}

	if (requirement->m_activityListItem.is_linked()) {
		requirement->m_activityListItem.unlink();
	} else {
		LOG_AM_DEBUG("Found unlinked requirement marking [Requirement %s] as met for [Activity %llu]",
			requirement->GetName().c_str(), m_id);
	}

	m_metRequirements.push_back(*requirement);

	/* Not obvious from here it'll be time to start.
	 * XXX use plug/unplug and the fact that an event will only be queued
	 * if another isn't already... */
	BroadcastEvent(ActivityUpdateEvent);

	if (m_unmetRequirements.empty()) {
		LOG_AM_DEBUG("[Activity %llu] All requirements met", m_id);

		if (!m_running && !m_ready && IsRunnable()) {
			RequestRunActivity();
		}
	}
}

void Activity::RequirementUnmet(boost::shared_ptr<Requirement> requirement)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] [Requirement %s] unmet", m_id,
		requirement->GetName().c_str());

	if (requirement->GetActivity() != shared_from_this()) {
		LOG_AM_ERROR(MSGID_REQ_UNMET_OWNER_MISMATCH,3,
			PMLOGKS("REQUIREMENT",requirement->GetName().c_str()),
            PMLOGKFV("Owned Activity","%llu",requirement->GetActivity()->GetId()),
			PMLOGKFV("Current Activity","%llu",m_id),
			"Activity id differ for unmet");
		throw std::runtime_error("Requirement owner mismatch");
	}

	if (requirement->m_activityListItem.is_linked()) {
		requirement->m_activityListItem.unlink();
	} else {
		LOG_AM_DEBUG("Found unlinked requirement marking [Requirement %s] as unmet for [Activity %llu]",
			requirement->GetName().c_str(), m_id);
	}

	m_unmetRequirements.push_back(*requirement);

	if (!m_running && m_ready) {
		m_ready = false;
		m_am.lock()->InformActivityNotReady(shared_from_this());
	}

	BroadcastEvent(ActivityUpdateEvent);
}

void Activity::RequirementUpdated(boost::shared_ptr<Requirement> requirement)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] [Requirement %s] updated", m_id,
		requirement->GetName().c_str());

	if (requirement->GetActivity() != shared_from_this()) {
		LOG_AM_ERROR(MSGID_UNOWNED_UPDATED_REQUIREMENT, 2, PMLOGKFV("Activity","%llu",m_id),
			  PMLOGKS("Requirement",requirement->GetName().c_str()), "Updated requirement is not owned by activity");
		throw std::runtime_error("Requirement owner mismatch");
	}

	BroadcastEvent(ActivityUpdateEvent);
}

void Activity::SetParent(boost::shared_ptr<Subscription> sub)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Attempting to set %s as parent",
		m_id, sub->GetSubscriber().GetString().c_str());

	if (!m_parent.expired())
		throw std::runtime_error("Activity already has parent set");

	m_parent = sub;
	m_released = false;

	LOG_AM_DEBUG("[Activity %llu] %s assigned as parent", m_id,
		sub->GetSubscriber().GetString().c_str());
}

MojErr Activity::Adopt(boost::shared_ptr<Subscription> sub, bool wait, bool *adopted)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] %s attempting to adopt %s", m_id,
		sub->GetSubscriber().GetString().c_str(),
		wait ? "and willing to wait" : "");

	m_adopters.push_back(sub);

	if (!m_parent.expired()) {
		if (adopted)
			*adopted = false;

		if (!wait) {
			LOG_AM_DEBUG("[Activity %llu] already has a parent (%s), and %s does not want to wait",
				m_id,
				m_parent.lock()->GetSubscriber().GetString().c_str(),
				sub->GetSubscriber().GetString().c_str());

			return MojErrWouldBlock;
		}

		LOG_AM_DEBUG("[Activity %llu] %s added to adopter list",
			m_id, sub->GetSubscriber().GetString().c_str());
	} else {
		if (adopted)
			*adopted = true;

		DoAdopt();
	}

	return MojErrNone;
}

MojErr Activity::Release(const BusId& caller)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] %s attempting to release", m_id,
		caller.GetString().c_str());

	/* Can't release if it's already been released. */
	if (m_released) {
		LOG_AM_DEBUG("[Activity %llu] Has already been released",
			m_id);
		return MojErrInvalidArg;
	}

	if (m_parent.lock()->GetSubscriber() != caller) {
		LOG_AM_DEBUG("[Activity %llu] %s failed to release, as %s is currently the parent",
			m_id, caller.GetString().c_str(),
			m_parent.lock()->GetSubscriber().GetString().c_str());
		return MojErrAccessDenied;
	}

	m_releasedParent = m_parent;
	m_parent.reset();
	m_released = true;

	LOG_AM_DEBUG("[Activity %llu] Released by %s", m_id,
		caller.GetString().c_str());

	if (!m_adopters.empty()) {
		DoAdopt();
	}

	return MojErrNone;
}

bool Activity::IsReleased() const
{
	return m_released;
}

MojErr Activity::Complete(const BusId& caller, bool force)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] %s attempting to complete", m_id,
		caller.GetString().c_str());

	if (!force && (m_released || m_parent.expired() ||
		(m_parent.lock()->GetSubscriber() != caller))
		&& (caller != m_creator))
		return MojErrAccessDenied;

	/* XXX Think about this one.  As per discussions, allowing complete
	 * as a signal to other participating processes may be useful */
	SendCommand(ActivityCompleteCommand);

	LOG_AM_DEBUG("[Activity %llu] Completed by %s", m_id,
		caller.GetString().c_str());

	return MojErrNone;
}

void Activity::SetPersistent(bool persistent)
{
	m_persistent = persistent;
}

bool Activity::IsPersistent() const
{
	return m_persistent;
}

void Activity::SetPersistToken(boost::shared_ptr<PersistToken> token)
{
	m_persistToken = token;
}

boost::shared_ptr<PersistToken> Activity::GetPersistToken()
{
	return m_persistToken;
}

void Activity::ClearPersistToken()
{
	m_persistToken.reset();
}

bool Activity::IsPersistTokenSet() const
{
	if (m_persistToken)
		return true;
	else
		return false;
}

void Activity::HookPersistCommand(boost::shared_ptr<PersistCommand> cmd)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Hooking [PersistCommand %s]", m_id,
		cmd->GetString().c_str());

	if (cmd->GetActivity()->GetId() != m_id) {
		LOG_AM_ERROR(MSGID_HOOK_PERSIST_CMD_ERR,3,
			PMLOGKS("persist_command",cmd->GetString().c_str()),
            PMLOGKFV("Assigned_activity","%llu",cmd->GetActivity()->GetId()),
            PMLOGKFV(" Current activity","%llu",m_id),
			"Attempt to hook persist command assigned to a different Activity");
		throw std::runtime_error("Attempt to hook persist command which "
			"is currently assigned to a different Activity");
	}

	m_persistCommands.push_back(cmd);
}

void Activity::UnhookPersistCommand(boost::shared_ptr<PersistCommand> cmd)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Unhooking [PersistCommand %s]", m_id,
		cmd->GetString().c_str());

	if (cmd->GetActivity()->GetId() != m_id) {
		LOG_AM_ERROR(MSGID_UNHOOK_CMD_ACTVTY_ERR, 3, PMLOGKFV("activity","%llu",m_id),
			  PMLOGKS("persist_command",cmd->GetString().c_str()),
			  PMLOGKFV("Assigned_activity","%llu",cmd->GetActivity()->GetId()),
			  "Attempt to unhook PersistCommand assigned to different activity");
		throw std::runtime_error("Attempt to unhook persist command which "
			"is currently assigned to a different Activity");
	}

	if (cmd != m_persistCommands.front()) {
		CommandQueue::iterator found = std::find(m_persistCommands.begin(),
			m_persistCommands.end(), cmd);
		if (found != m_persistCommands.end()) {
			LOG_AM_WARNING(MSGID_UNHOOK_CMD_QUEUE_ORDERING_ERR, 2, PMLOGKFV("Activity","%llu",m_id),
				  PMLOGKS("PersistCommand",cmd->GetString().c_str()),
				  "Request to unhook persistCommand which is not the first persist command in the queue");
			m_persistCommands.erase(found);
		} else {
			LOG_AM_WARNING(MSGID_UNHOOK_CMD_NOT_IN_QUEUE, 2,
				PMLOGKFV("Activity","%llu",m_id),
				PMLOGKS("PersistCommand",cmd->GetString().c_str()),
				  "PersistCommand Not in the queue");
		}
	} else {
		m_persistCommands.pop_front();

		/* If any subscriptions were holding events... */
		if (m_persistCommands.empty()) {
			UnplugAllSubscriptions();
		}
	}

	if (m_ending && m_persistCommands.empty()) {
		EndActivity();
	}
}

bool Activity::IsPersistCommandHooked() const
{
	return (!m_persistCommands.empty());
}

boost::shared_ptr<PersistCommand> Activity::GetHookedPersistCommand()
{
	if (m_persistCommands.empty()) {
		LOG_AM_ERROR(MSGID_HOOKED_PERSIST_CMD_NOT_FOUND, 1, PMLOGKFV("activity","%llu",m_id),
			  "Attempt to retreive the current hooked persist command, but none is present");
		throw std::runtime_error("Attempt to retreive hooked persist command, "
			"but none is present");
	}

	return m_persistCommands.front();
}

void Activity::SetPriority(ActivityPriority_t priority)
{
	m_priority = priority;
}

ActivityPriority_t Activity::GetPriority() const
{
	return m_priority;
}

void Activity::SetFocus(bool focused)
{
	if (m_focused == focused) {
		return;
	}

	m_focused = focused;

	/* Focus changed, update the sorting of all the Associations */
	std::for_each(m_associations.begin(), m_associations.end(),
		boost::mem_fn(&ActivitySetAutoAssociation::Reassociate));

	if (focused) {
		BroadcastEvent(ActivityFocusedEvent);
	} else {
		BroadcastEvent(ActivityUnfocusedEvent);
	}
}

bool Activity::IsFocused() const
{
	return m_focused;
}

void Activity::SetExplicit(bool explicitActivity)
{
	m_explicit = explicitActivity;
}

bool Activity::IsExplicit() const
{
	return m_explicit;
}

void Activity::SetTerminateFlag(bool terminate)
{
	m_terminate = terminate;
}

void Activity::SetRestartFlag(bool restart)
{
	m_restart = restart;
}

void Activity::SetUseSimpleType(bool useSimpleType)
{
	m_useSimpleType = useSimpleType;
}

void Activity::SetBusType(BusType bus)
{
	m_bus = bus;
}

Activity::BusType Activity::GetBusType() const
{
	return m_bus;
}

void Activity::SetContinuous(bool continuous)
{
	m_continuous = continuous;
}

bool Activity::IsContinuous() const
{
	return m_continuous;
}

void Activity::SetUserInitiated(bool userInitiated)
{
	m_userInitiated = userInitiated;
}

bool Activity::IsUserInitiated() const
{
	return m_userInitiated;
}

void Activity::SetPowerActivity(boost::shared_ptr<PowerActivity> powerActivity)
{
	m_powerActivity = powerActivity;
}

boost::shared_ptr<PowerActivity> Activity::GetPowerActivity()
{
	return m_powerActivity;
}

bool Activity::IsPowerActivity() const
{
	return m_powerActivity;
}

void Activity::PowerLockedNotification()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Received notification that power has been successfully locked on",
		m_id);

	m_powerActivity->GetManager()->ConfirmPowerActivityBegin(
		shared_from_this());

	if (!m_ending) {
		DoRunActivity();
	}
}

void Activity::PowerUnlockedNotification()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Received notification that power has been successfully unlocked",
		m_id);

	m_powerActivity->GetManager()->ConfirmPowerActivityEnd(
		shared_from_this());

	if (m_ending) {
		EndActivity();
	}
}

void Activity::SetPowerDebounce(bool powerDebounce)
{
	m_powerDebounce = powerDebounce;
}

bool Activity::IsPowerDebounce() const
{
	return m_powerDebounce;
}

void Activity::SetMetadata(const std::string& metadata)
{
	m_metadata = metadata;
}

void Activity::ClearMetadata()
{
	m_metadata.clear();
}

/* Automatic Association Management */
void Activity::AddEntityAssociation(
	boost::shared_ptr<ActivitySetAutoAssociation> association)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] adding association with [Entity %s]",
		m_id, association->GetTargetName().c_str());

	m_associations.insert(association);
}

void Activity::RemoveEntityAssociation(
	boost::shared_ptr<ActivitySetAutoAssociation> association)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] removing association with [Entity %s]",
		m_id, association->GetTargetName().c_str());

	EntityAssociationSet::iterator found = m_associations.find(association);
	if (found == m_associations.end()) {
		LOG_AM_WARNING(MSGID_RM_ASSOCIATION_NOT_FOUND,2,
			PMLOGKFV("Activity","%llu",m_id),
			PMLOGKS("Entity",association->GetTargetName().c_str()),
			"can't remove association with Entity");
		return;
	}

	m_associations.erase(found);
}

/*
 * IMPLEMENTATION --------------------------------------------
 */

/*
 * Commands in priority order: Cancel, Stop, Complete, Pause, and Start
 *
 * The Activity Manager itself will never decide to Start or Complete 
 * an Activity.  The Creater of the Activity needs to make that decision.
 */
ActivityCommand_t Activity::ComputeNextExternalCommand() const
{
	if ((m_intCommand == ActivityCancelCommand) ||
		(m_extCommand == ActivityCancelCommand))
		return ActivityCancelCommand;

	if ((m_intCommand == ActivityStopCommand) ||
		(m_extCommand == ActivityStopCommand))
		return ActivityStopCommand;

	if (m_extCommand == ActivityCompleteCommand)
		return ActivityCompleteCommand;

	if ((m_intCommand == ActivityPauseCommand) ||
		(m_extCommand == ActivityPauseCommand))
		return ActivityPauseCommand;

	if (m_extCommand == ActivityStartCommand)
		return ActivityStartCommand;

	return ActivityNoCommand;
}

void Activity::BroadcastCommand(ActivityCommand_t command)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Broadcasting \"%s\" command", m_id,
		ActivityCommandNames[command]);

	if (command == ActivityStartCommand)
		BroadcastEvent(ActivityStartEvent);
	else if (command == ActivityStopCommand)
		BroadcastEvent(ActivityStopEvent);
	else if (command == ActivityCancelCommand)
		BroadcastEvent(ActivityCancelEvent);
	else if (command == ActivityPauseCommand)
		BroadcastEvent(ActivityPauseEvent);
	else if (command == ActivityCompleteCommand)
		BroadcastEvent(ActivityCompleteEvent);
	else
		throw std::runtime_error("Request to Broadcast unknown command");

	m_sentCommand = command;
}

void Activity::RestartActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Restarting", m_id);

	m_initialized = false;
	m_scheduled = false;
	m_ready = false;
	m_running = false;
	m_ending = false;
	m_restart = false;
	m_requeue = false;
	m_yielding = false;

	/* If there are any associations, drop them. */
	/* XXX Consider whether to share this code with the Requeue logic */
	if (!m_associations.empty()) {
		LOG_AM_DEBUG("[Activity %llu] still has associations when restarting... clearing",
			m_id);
		m_associations.clear();
	}

	if (m_focused) {
		m_focused = false;
		if (m_focusedListItem.is_linked()) {
			m_focusedListItem.unlink();
		} else {
			LOG_AM_DEBUG("[Activity %llu] was focused but not on the focused Activities list", m_id);
		}
	} else {
		if (m_focusedListItem.is_linked()) {
			m_focusedListItem.unlink();
			LOG_AM_DEBUG("[Activity %llu] was unfocused but not on the focused Activities list", m_id);
		}
	}

	m_intCommand = ActivityNoCommand;
	m_extCommand = ActivityNoCommand;
	m_sentCommand = ActivityNoCommand;

	/* Re-issue start command */
	SendCommand(ActivityStartCommand);
}

void Activity::RequestScheduleActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Requesting permission to schedule from Activity Manager",
		m_id);

	m_initialized = true;
	m_am.lock()->InformActivityInitialized(shared_from_this());
}

void Activity::ScheduleActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Received permission to schedule",
		m_id);

	if (m_trigger) {
		m_trigger->Arm(shared_from_this());
	}

	if (m_schedule) {
		m_schedule->Queue();
	}

	m_scheduled = true;

	if (IsRunnable()) {
		RequestRunActivity();
	}
}

void Activity::RequestRunActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Requesting permission to run from Activity Manager",
		m_id);

	m_ready = true;
	m_am.lock()->InformActivityReady(shared_from_this());
}

void Activity::RunActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Received permission to run", m_id);

	m_running = true;

	if (m_powerActivity && (m_powerActivity->GetPowerState() !=
		PowerActivity::PowerLocked)) {
		LOG_AM_DEBUG("[Activity %llu] Requesting power be locked on",
			m_id);

		/* Request power be locked on, wait to actually start Activity until
		 * the power callback informs the Activity this has been accomplished */
		m_powerActivity->GetManager()->RequestBeginPowerActivity(
			shared_from_this());
	} else {
		DoRunActivity();
	}
}

void Activity::RequestRequeueActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Preparing to requeue", m_id);

	m_requeue = true;

	/* End this particular iteration of the Activity, but start from the
	 * scheduled point, rather than all the way at the top */
	EndActivity();
}

void Activity::RequeueActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Requeuing", m_id);

	m_running = false;
	m_ending = false;
	m_restart = false;
	m_requeue = false;
	m_yielding = false;

	if (IsRunnable()) {
		RequestRunActivity();
	} else {
		/* Requirements aren't met, or the Activity is paused. */
		m_ready = false;
		m_am.lock()->InformActivityNotReady(shared_from_this());
	}
}

void Activity::YieldActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);

	if (!m_yielding) {
		LOG_AM_DEBUG("[Activity %llu] Yielding", m_id);

		m_yielding = true;
		m_requeue = true;

		BroadcastEvent(ActivityYieldEvent);
		EndActivity();
	} else {
		LOG_AM_DEBUG("[Activity %llu] Already yielding",m_id);
	}
}

/*
 * If scheduled, remove from the queue.
 */
void Activity::EndActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Ending", m_id);

	if (!m_ending) {
		m_ending = true;
		m_am.lock()->InformActivityEnding(shared_from_this());
	}

	bool requeue = ShouldRequeue();

	/* Don't play with the trigger or schedule if requeuing... we want to
	 * save that state. */
	if (!requeue) {
		if (m_trigger && m_trigger->IsArmed(shared_from_this())) {
			m_trigger->Disarm(shared_from_this());
		}

		if (m_schedule && m_schedule->IsQueued()) {
			m_schedule->UnQueue();
		}
	}

	/* If there are no subscriptions, and power unlock is not still pending,
	 * check for restart, otherwise unregister. */
	if (m_subscriptions.empty()) {
		/* If the power is still locked on, begin unlock and wait for it to
		 * complete.  This may result in the function being called
		 * immediately from End() if direct power unlock is possible.
		 * That's ok.
		 * 
		 * Note: Power unlock shouldn't be initiated until after all the
		 * subscribers exit
		 */
		if (m_powerActivity && (m_powerActivity->GetPowerState() !=
			PowerActivity::PowerUnlocked)) {
			m_powerActivity->GetManager()->RequestEndPowerActivity(
				shared_from_this());
		} else if (m_persistCommands.empty()) {
			/* Don't move on to restarting (a potentially updated Activity)
 			 * until the updates have gone through. */

			/* It's ok for the Activity to restart after this.  Just let the
			 * Activity Manager know that that this incarnation of the Activity
			 * is done */
			m_am.lock()->InformActivityEnd(shared_from_this());

			if (requeue) {
				RequeueActivity();
			} else {
				/* Now we're really restarting, so advance the schedule */
				if (m_schedule) {
					m_schedule->InformActivityFinished();
				}

				bool restart = ShouldRestart();

				if (restart) {
					RestartActivity();
				} else {
					m_am.lock()->ReleaseActivity(shared_from_this());
				}
			}
		}
	}
}

/*
 * Any Triggers have fired, Requirements are met, it is Scheduled (and not
 * ending already)
 */
bool Activity::IsRunnable() const
{
	if (m_scheduled && !m_ending) {
		if (ComputeNextExternalCommand() != ActivityStartCommand)
			return false;

		if (m_trigger && !m_trigger->IsTriggered(shared_from_this()))
			return false;

		if (m_schedule && !m_schedule->IsScheduled())
			return false;

		if (!m_unmetRequirements.empty())
			return false;

		return true;
	}

	return false;
}

bool Activity::IsRunning() const
{
	if (m_running && !m_ending) {
		return true;
	} else {
		return false;
	}
}

bool Activity::IsYielding() const
{
	if (m_yielding) {
		return true;
	} else {
		return false;
	}
}

bool Activity::ShouldRestart() const
{
	if (m_callback && !m_terminate && (m_restart || m_persistent ||
			m_explicit || (m_schedule && m_schedule->ShouldReschedule()))) {
		return true;
	} else {
		return false;
	}
}

bool Activity::ShouldRequeue() const
{
	if (m_callback && !m_terminate && m_requeue && !m_restart) {
		return true;
	} else {
		return false;
	}
}

void Activity::Orphaned()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Orphaned", m_id);

	if (m_ending) {
		/* No particular action... waiting for all subscriptions to cancel */
	} else if (m_running) {
		/* Activity should Cancel.  If persistent, it may re-queue */
		SendCommand(ActivityCancelCommand, true);
	} else if (m_scheduled) {
		if (m_callback) {
			/* Activity is not running yet, but its callback will be used
			 * (ultimately to aquire a parent) when it does start. */
		} else {
			/* No adopters or way to aquire a new parent, so cancel. */
			SendCommand(ActivityCancelCommand, true);
		}
	} else {
		/* Was never fully initialized.  May have subscriptions/subscribers
		 * who should be notified, so Cancel. */
		SendCommand(ActivityCancelCommand, true);
	}
}

void Activity::Abandoned()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Abandoned (no subscribers remaining)", m_id);

	EndActivity();
}

void Activity::DoAdopt()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Attempting to find new parent",
		m_id);

	if (m_adopters.empty())
		throw std::runtime_error("No parents available for adoption");

	/* If the old parent is still waiting for notification of the adoption,
	 * inform them and drop the reference */
	if (!m_releasedParent.expired()) {
		m_releasedParent.lock()->QueueEvent(ActivityAdoptedEvent);
		m_releasedParent.reset();
	}

	m_parent = m_adopters.front();
	m_adopters.pop_front();

	m_parent.lock()->QueueEvent(ActivityOrphanEvent);

	m_released = false;
	if(m_ending) {
		LOG_AM_DEBUG("[Activity %llu] Removing ending flag since found new parent",m_id);
	}
	m_ending = false;

	LOG_AM_DEBUG("[Activity %llu] Adopted by %s", m_id,
		m_parent.lock()->GetSubscriber().GetString().c_str());
}

void Activity::DoRunActivity()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Running", m_id);

	/* Power is now on, if Applicable.  Tell the Activity Manager we're
	 * ready to go! */
	m_am.lock()->InformActivityRunning(shared_from_this());

	BroadcastCommand(ActivityStartCommand);

	if (m_callback) {
		DoCallback();
	}
}

void Activity::DoCallback()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Attempting to generate callback",
		m_id);

	if (!m_callback)
		throw std::runtime_error("No callback exists for Activity");

	MojErr err = m_callback->Call();

	if (err) {
		LOG_AM_ERROR(MSGID_ACTIVITY_CB_FAIL,1,
			PMLOGKFV("Activity","%llu",m_id),
			"Attempt to call callback failed");
		throw std::runtime_error("Failed to call Activity Callback");
	}
}

ActivityState_t Activity::GetState() const
{
	if (m_ending) {
		if (m_subscriptions.empty()) {
			switch (m_sentCommand) {
			case ActivityCancelCommand:
				return ActivityCancelled;
			case ActivityStopCommand:
				return ActivityStopped;
			case ActivityCompleteCommand:
				return ActivityComplete;
			default:
				return ActivityUnknown;	/* Don't throw and cause trouble */
			}
		} else {
			switch (m_sentCommand) {
			case ActivityCancelCommand:
				return ActivityCancelling;
			case ActivityStopCommand:
				return ActivityStopping;
			case ActivityCompleteCommand:
				return ActivityComplete;
			default:
				return ActivityUnknown;
			}
		}
	}

	if (m_running) {
		if (m_sentCommand == ActivityPauseCommand) {
			return ActivityPaused;
		} else if (m_sentCommand == ActivityStartCommand) {
			return ActivityRunning;
		} else {
			return ActivityStarting;
		}
	}

	if (m_scheduled) {
		if (!m_unmetRequirements.empty()) {
			return ActivityBlocked;
		} else if (m_ready && !m_running) {
			/* m_running check is redundant, but leave in to be safe in case of
			 * later changes */
			return ActivityQueued;
		} else {
			/* waiting for start command */
			return ActivityWaiting;
		}
	}

	return ActivityInit;
}

MojErr Activity::IdentityToJson(MojObject& rep) const
{
	/* Populate a JSON object with the minimal but important identifying
	 * information for an Activity - id, name, and creator */

	MojErr errs = MojErrNone;

	MojErr err = rep.putInt(_T("activityId"), (MojInt64)m_id);
	MojErrAccumulate(errs, err);

	err = rep.putString(_T("name"), m_name.c_str());
	MojErrAccumulate(errs, err);

	MojObject creator(MojObject::TypeObject);
	err = m_creator.ToJson(creator);
	MojErrAccumulate(errs, err);

	err = rep.put(_T("creator"), creator);
	MojErrAccumulate(errs, err);

	return errs;
}

MojErr Activity::ActivityInfoToJson(MojObject& rep) const
{
	/* Populate "$activity" property with markup about the Activity.
	 * This should include the activityId and the output of the Trigger,
	 * if present.
	 *
	 * XXX it should also include error information if the Activity failed to
	 * complete and had to be re-started, or if the Activity's callback failed
	 * to result in an Adoption. */

	MojErr err;

	err = rep.putInt(_T("activityId"), (MojInt64)m_id);
	MojErrCheck(err);

	if (HasCallback()) {
		MojObject callbackRep;
		err = m_callback->ToJson(callbackRep, ACTIVITY_JSON_CURRENT);
		MojErrCheck(err);

		err = rep.put(_T("callback"), callbackRep);
		MojErrCheck(err);
	}

	if (HasTrigger()) {
		MojObject triggerRep;
		err = TriggerToJson(triggerRep, ACTIVITY_JSON_CURRENT);
		MojErrCheck(err);

		err = rep.put(_T("trigger"), triggerRep);
		MojErrCheck(err);
	}

	if (HasRequirements()) {
		MojObject requirementsRep;
		err = RequirementsToJson(requirementsRep, ACTIVITY_JSON_CURRENT);
		MojErrCheck(err);

		err = rep.put(_T("requirements"), requirementsRep);
		MojErrCheck(err);
	}

	MojObject creator;
	err = m_creator.ToJson(creator);
	MojErrCheck(err);

	err = rep.put(_T("creator"), creator);
	MojErrCheck(err);

	err = rep.putString(_T("name"), m_name.c_str());
	MojErrCheck(err);

	if (!m_metadata.empty()) {
		MojObject metadataJson;
		err = metadataJson.fromJson(m_metadata.c_str());
		MojErrCheck(err);

		err = rep.put(_T("metadata"), metadataJson);
		MojErrCheck(err);
	}

	return MojErrNone;
}

MojErr Activity::ToJson(MojObject& rep, unsigned flags) const
{
	MojErr err = MojErrNone;

	err = rep.putInt(_T("activityId"), (MojInt64)m_id);
	MojErrCheck(err);

	err = rep.putString(_T("name"), m_name.c_str());
	MojErrCheck(err);

	err = rep.putString(_T("description"), m_description.c_str());
	MojErrCheck(err);

	if (!m_metadata.empty()) {
		MojObject metadataJson;
		err = metadataJson.fromJson(m_metadata.c_str());
		MojErrCheck(err);

		err = rep.put(_T("metadata"), metadataJson);
		MojErrCheck(err);
	}

	MojObject creator(MojObject::TypeObject);
	err = m_creator.ToJson(creator);
	MojErrCheck(err);

	err = rep.put(_T("creator"), creator);
	MojErrCheck(err);

	if (!(flags & ACTIVITY_JSON_PERSIST)) {
		err = rep.putBool(_T("focused"), m_focused);
		MojErrCheck(err);

		err = rep.putString(_T("state"), GetStateString().c_str());
		MojErrCheck(err);

		if (flags & ACTIVITY_JSON_SUBSCRIBERS) {
			if (!m_parent.expired()) {
				MojObject subscriber(MojObject::TypeObject);

				err = m_parent.lock()->GetSubscriber().ToJson(subscriber);
				MojErrCheck(err);

				err = rep.put(_T("parent"), subscriber);
				MojErrCheck(err);
			}

			MojObject subscriberArray(MojObject::TypeArray);
			for (SubscriptionSet::const_iterator iter =
				m_subscriptions.begin();
				iter != m_subscriptions.end(); ++iter) {
				MojObject subscriber(MojObject::TypeObject);

				err = iter->GetSubscriber().ToJson(subscriber);
				MojErrCheck(err);

				err = subscriberArray.push(subscriber);
				MojErrCheck(err);
			}

			err = rep.put(_T("subscribers"), subscriberArray);
			MojErrCheck(err);

			MojObject adopterArray(MojObject::TypeArray);
			for (AdopterQueue::const_iterator iter =
				m_adopters.begin(); iter != m_adopters.end(); ++iter) {
				MojObject subscriber(MojObject::TypeObject);

				err = iter->lock()->GetSubscriber().ToJson(subscriber);
				MojErrCheck(err);

				err = adopterArray.push(subscriber);
				MojErrCheck(err);
			}

			err = rep.put(_T("adopters"), adopterArray);
			MojErrCheck(err);
		}
	}

	if (flags & ACTIVITY_JSON_DETAIL) {
		MojObject type;

		err = TypeToJson(type, flags);
		MojErrCheck(err);

		err = rep.put(_T("type"), type);
		MojErrCheck(err);

		if (m_callback) {
			MojObject callbackRep;

			err = m_callback->ToJson(callbackRep, flags);
			MojErrCheck(err);

			err = rep.put(_T("callback"), callbackRep);
			MojErrCheck(err);
		}

		if (m_schedule) {
			MojObject scheduleRep;

			err = m_schedule->ToJson(scheduleRep, flags);
			MojErrCheck(err);

			err = rep.put(_T("schedule"), scheduleRep);
			MojErrCheck(err);
		}

		if (m_trigger) {
			MojObject triggerRep;

			err = m_trigger->ToJson(shared_from_this(), triggerRep, flags);
			MojErrCheck(err);

			err = rep.put(_T("trigger"), triggerRep);
			MojErrCheck(err);
		}

		if (!m_metRequirements.empty() || !m_unmetRequirements.empty()) {
			MojObject requirementsRep;

			err = RequirementsToJson(requirementsRep, flags);
			MojErrCheck(err);

			err = rep.put(_T("requirements"), requirementsRep);
			MojErrCheck(err);
		}
	}

	if (flags & ACTIVITY_JSON_INTERNAL) {
		MojObject internalRep;

		err = InternalToJson(internalRep, flags);
		MojErrCheck(err);

		err = rep.put(_T("internal"), internalRep);
		MojErrCheck(err);
	}

	return MojErrNone;
}

MojErr Activity::TypeToJson(MojObject& rep, unsigned flags) const
{
	MojErr err;

	if (m_bus == PublicBus) {
		err = rep.putString(_T("bus"), "public");
		MojErrCheck(err);
	} else {
		err = rep.putString(_T("bus"), "private");
		MojErrCheck(err);
	}

	if (m_persistent) {
		err = rep.putBool(_T("persist"), true);
		MojErrCheck(err);
	}

	if (m_explicit) {
		err = rep.putBool(_T("explicit"), true);
		MojErrCheck(err);
	}

	bool useSimpleType = m_useSimpleType;

	if (useSimpleType) {
		if (m_immediate && (m_priority == ActivityForegroundPriority)) {
			err = rep.putBool(_T("foreground"), true);
			MojErrCheck(err);
		} else if (!m_immediate && (m_priority == ActivityBackgroundPriority)) {
			err = rep.putBool(_T("background"), true);
			MojErrCheck(err);
		} else {
			LOG_AM_DEBUG(_T("[Activity %llu] Simple Activity type "
			    "specified, but immediate and priority were individually set, "
                "and will be individually output"), m_id);
			useSimpleType = false;
		}
	}

	if (!useSimpleType) {
		err = rep.putBool(_T("immediate"), m_immediate);
		MojErrCheck(err);

		err = rep.putString(_T("priority"),
			ActivityPriorityNames[m_priority]);
		MojErrCheck(err);
	}

	if (m_continuous) {
		err = rep.putBool(_T("continuous"), true);
		MojErrCheck(err);
	}

	if (m_userInitiated) {
		err = rep.putBool(_T("userInitiated"), true);
		MojErrCheck(err);
	}

	if (m_powerActivity) {
		err = rep.putBool(_T("power"), true);
		MojErrCheck(err);
	}

	if (m_powerDebounce) {
		err = rep.putBool(_T("powerDebounce"), true);
		MojErrCheck(err);
	}

	return MojErrNone;
}

MojErr Activity::TriggerToJson(MojObject& rep, unsigned flags) const
{
	return m_trigger->ToJson(shared_from_this(), rep, flags);
}

MojErr Activity::RequirementsToJson(MojObject& rep, unsigned flags) const
{
	MojErr err;

	err = MetRequirementsToJson(rep, flags);
	MojErrCheck(err);

	err = UnmetRequirementsToJson(rep, flags);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr Activity::MetRequirementsToJson(MojObject& rep, unsigned flags) const
{
	/* XXX really should be catching errors.  ToJson should throw on
	 * allocation failure... */
	std::for_each(m_metRequirements.begin(), m_metRequirements.end(),
		boost::bind(&Requirement::ToJson, _1, boost::ref(rep), flags));

	return MojErrNone;
}

MojErr Activity::UnmetRequirementsToJson(MojObject& rep, unsigned flags) const
{
	/* XXX really should be catching errors.  ToJson should throw on
	 * allocation failure... */
	std::for_each(m_unmetRequirements.begin(), m_unmetRequirements.end(),
		boost::bind(&Requirement::ToJson, _1, boost::ref(rep), flags));

	return MojErrNone;
}

MojErr Activity::InternalToJson(MojObject& rep, unsigned flags) const
{
	MojErr err;

	err = rep.putInt(_T("m_id"), (MojInt64)m_id);
	MojErrCheck(err);

	err = rep.putBool(_T("m_persistent"), m_persistent);
	MojErrCheck(err);

	err = rep.putBool(_T("m_explicit"), m_explicit);
	MojErrCheck(err);

	err = rep.putBool(_T("m_immediate"), m_immediate);
	MojErrCheck(err);

	err = rep.putInt(_T("m_priority"), (MojInt64)m_priority);
	MojErrCheck(err);

	err = rep.putBool(_T("m_useSimpleType"), m_useSimpleType);
	MojErrCheck(err);

	err = rep.putBool(_T("m_focused"), m_focused);
	MojErrCheck(err);

	err = rep.putBool(_T("m_continuous"), m_continuous);
	MojErrCheck(err);

	err = rep.putBool(_T("m_userInitiated"), m_userInitiated);
	MojErrCheck(err);
	
	err = rep.putBool(_T("m_powerDebounce"), m_powerDebounce);
	MojErrCheck(err);

	err = rep.putInt(_T("m_bus"), (MojInt64)m_bus);
	MojErrCheck(err);

	err = rep.putBool(_T("m_initialized"), m_initialized);
	MojErrCheck(err);

	err = rep.putBool(_T("m_scheduled"), m_scheduled);
	MojErrCheck(err);

	err = rep.putBool(_T("m_ready"), m_ready);
	MojErrCheck(err);

	err = rep.putBool(_T("m_running"), m_running);
	MojErrCheck(err);

	err = rep.putBool(_T("m_ending"), m_ending);
	MojErrCheck(err);

	err = rep.putBool(_T("m_terminate"), m_terminate);
	MojErrCheck(err);

	err = rep.putBool(_T("m_restart"), m_restart);
	MojErrCheck(err);

	err = rep.putBool(_T("m_requeue"), m_requeue);
	MojErrCheck(err);

	err = rep.putBool(_T("m_yielding"), m_yielding);
	MojErrCheck(err);

	err = rep.putBool(_T("m_released"), m_released);
	MojErrCheck(err);

	err = rep.putString(_T("m_intCommand"), ActivityCommandNames[m_intCommand]);
	MojErrCheck(err);

	err = rep.putString(_T("m_extCommand"), ActivityCommandNames[m_extCommand]);
	MojErrCheck(err);

	err = rep.putString(_T("m_sentCommand"), ActivityCommandNames[m_sentCommand]);
	MojErrCheck(err);

	return MojErrNone;
}

void Activity::PushIdentityJson(MojObject& array) const
{
	MojErr errs = MojErrNone;

	MojObject identity(MojObject::TypeObject);

	MojErr err = IdentityToJson(identity);
	MojErrAccumulate(errs, err);

	err = array.push(identity);
	MojErrAccumulate(errs, err);

	if (errs) {
		throw std::runtime_error("Unable to convert from Activity to JSON "
			"object");
	}
}
