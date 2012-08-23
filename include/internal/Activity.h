/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef __ACTIVITYMANAGER_ACTIVITY_H__
#define __ACTIVITYMANAGER_ACTIVITY_H__

#include <set>
#include <map>
#include <list>
#include <string>
#include <vector>

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/list.hpp>

#include "Base.h"
#include "ActivityTypes.h"
#include "BusId.h"
#include "Subscriber.h"
#include "Subscription.h"
#include "PersistCommand.h"
#include "Requirement.h"
#include "Callback.h"

#include <core/MojServiceMessage.h>

class Subscription;
class ActivityManager;
class Trigger;
class Schedule;
class PersistToken;
class PowerActivity;
class ActivitySetAutoAssociation;

class Activity : public boost::enable_shared_from_this<Activity>
{
public:
	Activity(activityId_t id, boost::weak_ptr<ActivityManager> am);
	virtual ~Activity();

	/* Accessors for external properties */
	activityId_t GetId() const;

	void SetName(const std::string& name);
	const std::string& GetName() const;
	bool IsNameRegistered() const;

	void SetDescription(const std::string& description);
	const std::string& GetDescription() const;

	void SetCreator(const Subscriber& creator);
	void SetCreator(const BusId& creator);
	const BusId& GetCreator() const;

	void SetImmediate(bool immediate);
	bool IsImmediate() const;

	std::string GetStateString() const;

	void SendCommand(ActivityCommand_t command, bool internal = false);

	/* Subscription/Subscriber management */
	MojErr AddSubscription(boost::shared_ptr<Subscription> sub);
	MojErr RemoveSubscription(boost::shared_ptr<Subscription> sub);
	bool IsSubscribed() const;
	typedef std::vector<BusId> SubscriberIdVec;
	SubscriberIdVec GetUniqueSubscribers() const;

	/* Subscription messaging & control */
	MojErr BroadcastEvent(ActivityEvent_t event);
	void PlugAllSubscriptions();
	void UnplugAllSubscriptions();

	/* Trigger Management */
	void SetTrigger(boost::shared_ptr<Trigger> trigger);
	boost::shared_ptr<Trigger> GetTrigger() const;
	void ClearTrigger();
	bool HasTrigger() const;
	void Triggered(boost::shared_ptr<Trigger> trigger);
	bool IsTriggered() const;

	/* Callback Management */
	void SetCallback(boost::shared_ptr<Callback> callback);
	boost::shared_ptr<Callback> GetCallback();
	bool HasCallback() const;
	void CallbackFailed(boost::shared_ptr<Callback> callback,
		Callback::FailureType failure);
	void CallbackSucceeded(boost::shared_ptr<Callback> callback);

	/* Schedule Management */
	void SetSchedule(boost::shared_ptr<Schedule> schedule);
	void ClearSchedule();
	void Scheduled();
	bool IsScheduled() const;

	/* Requirement Management */
	void AddRequirement(boost::shared_ptr<Requirement> requirement);
	void RemoveRequirement(boost::shared_ptr<Requirement> requirement);
	void RemoveRequirement(const std::string& name);
	bool IsRequirementSet(const std::string& name) const;
	bool HasRequirements() const;
	void RequirementMet(boost::shared_ptr<Requirement> requirement);
	void RequirementUnmet(boost::shared_ptr<Requirement> requirement);
	void RequirementUpdated(boost::shared_ptr<Requirement> requirement);

	/* Parent Management */
	void SetParent(boost::shared_ptr<Subscription> sub);
	MojErr Adopt(boost::shared_ptr<Subscription> sub, bool wait, bool *adopted = NULL);
	MojErr Release(const BusId& caller);
	bool IsReleased() const;

	MojErr Complete(const BusId& caller, bool force = false);

	/* Persistence Management */
	void SetPersistent(bool persistent);
	bool IsPersistent() const;

	void SetPersistToken(boost::shared_ptr<PersistToken> token);
	boost::shared_ptr<PersistToken> GetPersistToken();
	void ClearPersistToken();
	bool IsPersistTokenSet() const;

	void HookPersistCommand(boost::shared_ptr<PersistCommand> cmd);
	void UnhookPersistCommand(boost::shared_ptr<PersistCommand> cmd);
	bool IsPersistCommandHooked() const;
	boost::shared_ptr<PersistCommand> GetHookedPersistCommand();

	/* Priority management */
	void SetPriority(ActivityPriority_t priority);
	ActivityPriority_t GetPriority() const;

	/* Focus management */
	void SetFocus(bool focused);
	bool IsFocused() const;

	/* Explicit status Management */
	void SetExplicit(bool explicitActivity);
	bool IsExplicit() const;

	void SetTerminateFlag(bool terminate);
	void SetRestartFlag(bool restart);

	/* Use simple 'foreground' or 'background' */
	void SetUseSimpleType(bool useSimpleType);

	/* Track bus access */
	typedef enum BusType_e { PublicBus, PrivateBus } BusType;
	void SetBusType(BusType bus);
	BusType GetBusType() const;

	/* Additional hints: Continuous and User Initiated control */
	void SetContinuous(bool continuous);
	bool IsContinuous() const;

	void SetUserInitiated(bool userInitiated);
	bool IsUserInitiated() const;

	/* Power management control */
	void SetPowerActivity(boost::shared_ptr<PowerActivity> powerActivity);
	boost::shared_ptr<PowerActivity> GetPowerActivity();
	bool IsPowerActivity() const;
	void PowerLockedNotification();
	void PowerUnlockedNotification();

	/* Power debounce control */
	void SetPowerDebounce(bool powerDebounce);
	bool IsPowerDebounce() const;

	/* Activity external metadata manipulation */
	void SetMetadata(const std::string& metadata);
	void ClearMetadata();

	/* Dynamic auto-association management */
	void AddEntityAssociation(
		boost::shared_ptr<ActivitySetAutoAssociation> association);
	void RemoveEntityAssociation(
		boost::shared_ptr<ActivitySetAutoAssociation> association);

	MojErr IdentityToJson(MojObject& rep) const;
	MojErr ActivityInfoToJson(MojObject& rep) const;
	MojErr ToJson(MojObject& rep, unsigned flags) const;
	MojErr TypeToJson(MojObject& rep, unsigned flags) const;
	MojErr TriggerToJson(MojObject& rep, unsigned flags) const;
	MojErr RequirementsToJson(MojObject& rep, unsigned flags) const;
	MojErr MetRequirementsToJson(MojObject& rep, unsigned flags) const;
	MojErr UnmetRequirementsToJson(MojObject& rep, unsigned flags) const;
	MojErr InternalToJson(MojObject& rep, unsigned flags) const;

	void PushIdentityJson(MojObject& array) const;

	bool IsRunning() const;
	bool IsYielding() const;

private:
	/* Activity Manager may access "private" control interfaces. */
	friend class ActivityManager;

	ActivityCommand_t ComputeNextExternalCommand() const;
	void BroadcastCommand(ActivityCommand_t command);

	void RestartActivity();
	void RequestScheduleActivity();
	void ScheduleActivity();
	void RequestRunActivity();
	void RunActivity();
	void RequestRequeueActivity();
	void RequeueActivity();
	void YieldActivity();
	void EndActivity();

	bool IsRunnable() const;
	bool ShouldRestart() const;
	bool ShouldRequeue() const;

	void Orphaned();
	void Abandoned();

	void DoAdopt();
	void DoRunActivity();
	void DoCallback();

	/* State is a computed property based on internal state */
	ActivityState_t GetState(void) const;

	/* DISALLOW */
	Activity();
	Activity(const Activity& copy);
	Activity& operator=(const Activity& copy);
	bool operator==(const Activity& rhs) const;

private:
	typedef boost::intrusive::set_member_hook<
		boost::intrusive::link_mode<
			boost::intrusive::auto_unlink> > ActivityTableItem;

	typedef boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<
			boost::intrusive::auto_unlink> > ActivityListItem;

	typedef boost::intrusive::member_hook<Requirement,
		Requirement::RequirementListItem,
		&Requirement::m_activityListItem> RequirementListOption;
	typedef boost::intrusive::list<Requirement, RequirementListOption,
		boost::intrusive::constant_time_size<false> > RequirementList;

	typedef boost::intrusive::member_hook<Subscriber, Subscriber::SetItem,
		&Subscriber::m_item> SubscriberSetOption;
	typedef boost::intrusive::multiset<Subscriber, SubscriberSetOption,
		boost::intrusive::constant_time_size<false> > SubscriberSet;

	typedef boost::intrusive::member_hook<Subscription, Subscription::SetItem,
		&Subscription::m_item> SubscriptionSetOption;
	typedef boost::intrusive::set<Subscription, SubscriptionSetOption,
		boost::intrusive::constant_time_size<false> > SubscriptionSet;

	typedef std::list<boost::shared_ptr<PersistCommand> > CommandQueue;

	typedef std::list<boost::weak_ptr<Subscription> > AdopterQueue;

	typedef std::map<std::string, boost::shared_ptr<Requirement> >
		RequirementMap;

	typedef std::set<boost::shared_ptr<ActivitySetAutoAssociation> >
		EntityAssociationSet;

	/* External properties */
	std::string		m_name;
	std::string		m_description;
	activityId_t	m_id;
	BusId			m_creator;

	/* Externally supplied metadata in JSON format.  This will be stored
	 * and returned with the Activity, and allows for more flexible tagging
	 * and decoration of the Activity. */
	std::string		m_metadata;

	/* The Activity will be stored in the Open webOS database.  Any command
	 * that causes an update to the Activity's persisted state will wait until
	 * that update has been acknowledged by MojoDB before returning to the
	 * caller (or otherwise generating externally visible events). */
	bool			m_persistent;

	/* The Activity must be explicitly terminated by command, rather than
	 * implicitly cancelled if its parent unsubscribes.  If the parent of
	 * an explicit Activity unsubscribes, cancel events are generated to the
	 * remaining subscribers.  After all subscribers unsubscribe, the Activity
	 * will be automatically rescheduled to run again. */
	bool			m_explicit;

	/* Activity should begin running (callback called, 'start' event generated)
	 * as soon as its prerequisites are met, without delay.  Otherwise, the
	 * Activity Manager may delay the Activity while other Activities run,
	 * and may change the order that those Activities are executed in. */
	bool			m_immediate;

	/* Priority of Activity: "highest", "high", "normal", "low", "lowest".
	 * Generally not directly set - just "background" ("low"), or
	 * "foreground" ("immediate", "normal"). */
	ActivityPriority_t	m_priority;

	/* If 'immediate' and 'priority' were assigned jointly as 'foreground'
	 * or 'background'. */
	bool			m_useSimpleType;

	/* Activity is currently focused.  This value isn't persisted, and should
	 * be kept updated by the rest of the system */
	bool			m_focused;

	/* Activity is expected to run for a potentially indefinite anount of time.
	 * Do not defer scheduling other things.  Activity should be set for
	 * immediate scheduling, otherwise, it's a bug. */
	bool			m_continuous;

	/* Activity was directly user-initiated. */
	bool			m_userInitiated;

	/* Activity should support power debouncing */
	bool			m_powerDebounce;

	/* Whether the request to create the Activity was received on the public
	 * or private bus.  Outbound trigger calls will be made on the same
	 * bus, to prevent privilege escalation. */
	BusType			m_bus;

	/* Activity has been commanded to start, and requested permission to 
	 * schedule */	
	bool			m_initialized;

	/* Activity has been told to schedule itself by the Activity Manager */
	bool			m_scheduled;

	/* Activity has requested permission to run */
	bool			m_ready;

	/* Activity has started (prereqs met, dequeued if subject to queuing) */
	bool			m_running;

	/* Activity is ending (by command or abandonment) */
	bool			m_ending;

	/* Activity is ending due to explicit command */
	bool			m_terminate;

	/* Activity has been requested to restart (via Complete) */
	bool			m_restart;

	/* Activity had an issue while attempting to start, and should return to
	 * the queue.  (Don't advance the Schedule, and don't lose the Trigger
	 * information) */
	bool			m_requeue;

	/* Activity has been told to yield. */
	bool			m_yielding;

	bool			m_released;

	ActivityCommand_t	m_intCommand;
	ActivityCommand_t	m_extCommand;
	ActivityCommand_t	m_sentCommand;

	AdopterQueue		m_adopters;
	SubscriptionSet		m_subscriptions;
	SubscriberSet		m_subscribers;

	RequirementMap		m_requirements;
	RequirementList		m_metRequirements;
	RequirementList		m_unmetRequirements;

	boost::weak_ptr<Subscription>	m_parent;
	boost::weak_ptr<Subscription>	m_releasedParent;

	boost::shared_ptr<Trigger>		m_trigger;
	boost::shared_ptr<Callback>		m_callback;
	boost::shared_ptr<Schedule>		m_schedule;

	boost::shared_ptr<PersistToken>	m_persistToken;

	/* Activity Manager should arbitrate with Power Management to keep device
	 * awake while Activity is running. */
	boost::shared_ptr<PowerActivity>	m_powerActivity;

	/* Associations other objects have to this Activity (that will auto-unlink
	 * when the association objects are deallocated when the Activity dies */
	EntityAssociationSet	m_associations;

	/* Index this in table of Activity names (and auto-unlink) */
	ActivityTableItem	m_nameTableItem;

	/* Index this in table of Activity IDs (and auto-unlink).  This includes
	 * Activities that have been released but still have references. */
	ActivityTableItem	m_idTableItem;

	/* Start/Ready/Run queue link */
	ActivityListItem	m_runQueueItem;

	/* List of Focused Activities */
	ActivityListItem	m_focusedListItem;

	/* List of pending Persist commands */
	CommandQueue		m_persistCommands;

	boost::weak_ptr<ActivityManager>	m_am;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_ACTIVITY_H__ */
