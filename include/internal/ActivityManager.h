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

#ifndef __ACTIVITYMANAGER_H__
#define __ACTIVITYMANAGER_H__

#include <map>
#include <vector>

#include "Base.h"
#include "Activity.h"
#include "Subscriber.h"
#include "Timeout.h"

/*
 * Central Activity registry and control object.
 */

class MasterResourceManager;

class ActivityManager : public boost::enable_shared_from_this<ActivityManager>
{
public:
	ActivityManager(boost::shared_ptr<MasterResourceManager> resourceManager);
	virtual ~ActivityManager();

	typedef std::map<activityId_t, boost::shared_ptr<Activity> > ActivityMap;
	typedef std::vector<boost::shared_ptr<const Activity> > ActivityVec;

	void RegisterActivityId(boost::shared_ptr<Activity> act);
	void RegisterActivityName(boost::shared_ptr<Activity> act);
	void UnregisterActivityName(boost::shared_ptr<Activity> act);

	boost::shared_ptr<Activity> GetActivity(activityId_t id);
	boost::shared_ptr<Activity> GetActivity(const std::string& name,
		const BusId& creator);

	boost::shared_ptr<Activity> GetNewActivity();
	boost::shared_ptr<Activity> GetNewActivity(activityId_t id);

	void ReleaseActivity(boost::shared_ptr<Activity> act);

	ActivityVec GetActivities() const;

	/* Activity Commands Interface */
	MojErr StartActivity(boost::shared_ptr<Activity> act);
	MojErr StopActivity(boost::shared_ptr<Activity> act);
	MojErr CancelActivity(boost::shared_ptr<Activity> act);
	MojErr PauseActivity(boost::shared_ptr<Activity> act);

	/* Focus Commands */
	MojErr FocusActivity(boost::shared_ptr<Activity> act);
	MojErr UnfocusActivity(boost::shared_ptr<Activity> act);
	MojErr AddFocus(boost::shared_ptr<Activity> source,
		boost::shared_ptr<Activity> target);

	/* Activity Manager Control Interface */
	static const unsigned EXTERNAL_ENABLE = 0x1;
	static const unsigned UI_ENABLE = 0x2;
	static const unsigned CONFIGURATION_LOADED = 0x4;

	static const unsigned ENABLE_MASK = EXTERNAL_ENABLE | UI_ENABLE |
		CONFIGURATION_LOADED;

	void Enable(unsigned mask);
	void Disable(unsigned mask);

	bool IsEnabled() const;

#ifdef ACTIVITYMANAGER_DEVELOPER_METHODS
	unsigned int SetBackgroundConcurrencyLevel(unsigned int level);
	void EvictBackgroundActivity(boost::shared_ptr<Activity> act);
	void EvictAllBackgroundActivities();
	void RunReadyBackgroundActivity(boost::shared_ptr<Activity> act);
	void RunAllReadyActivities();
#endif

	/* INTERFACE:  ACTIVITY ----> ACTIVITY MANAGER */

	/* The Activity is fully initialized and ready to be scheduled when
	 * Activity Manager requests that it do so */
	void InformActivityInitialized(boost::shared_ptr<Activity> act);

	/* The Activity's prerequisites have been met, and it's ready to run
	 * when the Activity Manager selects it to do so */
	void InformActivityReady(boost::shared_ptr<Activity> act);

	/* The Activity's prerequisites are not longer met.  It should be
	 * returned to the waiting queue */
	void InformActivityNotReady(boost::shared_ptr<Activity> act);

	/* The Activity has started running */
	void InformActivityRunning(boost::shared_ptr<Activity> act);

	/* The Activity has begun the process of ending and subscribers have
	 * been informed to unsubscribe */
	void InformActivityEnding(boost::shared_ptr<Activity> act);

	/* The Activity has ended.  If it wishes to restart, it will inform the
	 * Activity Manager it's reinitialized again */
	void InformActivityEnd(boost::shared_ptr<Activity> act);

	/* The Activity has gained or lost a unique subscriber (BusId). */
	void InformActivityGainedSubscriberId(boost::shared_ptr<Activity> act,
		const BusId& id);
	void InformActivityLostSubscriberId(boost::shared_ptr<Activity> act,
		const BusId& id);

	/* END INTERFACE  */

	static const unsigned DefaultBackgroundConcurrencyLevel = 1;
	static const unsigned DefaultBackgroundInteractiveConcurrencyLevel = 2;
	static const unsigned UnlimitedBackgroundConcurrency = 0;
	static const unsigned DefaultBackgroundInteractiveYieldSeconds = 60;

	/* Activity Manager info state gatherer */
	MojErr InfoToJson(MojObject& rep) const;

private:
	/* DISALLOW */
	ActivityManager(const ActivityManager& copy);
	ActivityManager& operator=(const ActivityManager& copy);

protected:
	void ScheduleAllActivities();
	void EvictQueue(boost::shared_ptr<Activity> act);
	void RunActivity(Activity& act);
	void RunReadyBackgroundActivity(Activity& act);
	void RunReadyBackgroundInteractiveActivity(Activity& act);
	unsigned GetRunningBackgroundActivitiesCount() const;
	void CheckReadyQueue();

	void UpdateYieldTimeout();
	void CancelYieldTimeout();
	void InteractiveYieldTimeout();

protected:
	typedef enum {
		RunQueueNone = -1,
		RunQueueInitialized = 0,
		RunQueueScheduled,
		RunQueueReady,
		RunQueueReadyInteractive,
		RunQueueBackground,
		RunQueueBackgroundInteractive,
		RunQueueLongBackground,
		RunQueueImmediate,
		RunQueueEnded,
		RunQueueMax
	} RunQueueId;

	/* Lightweight comparator object to search the Activity Name Table for
	 * a particular name */
	typedef std::pair<std::string, BusId>	ActivityKey;
	struct ActivityNameComp {
		bool operator()(const Activity& act1, const Activity& act2) const;
		bool operator()(const ActivityKey& key, const Activity& act) const;
		bool operator()(const Activity& act, const ActivityKey& key) const;
	};

	typedef boost::intrusive::member_hook<Activity, Activity::ActivityTableItem,
		&Activity::m_nameTableItem> ActivityNameTableOption;
	typedef boost::intrusive::set<Activity, ActivityNameTableOption,
		boost::intrusive::constant_time_size<false>,
		boost::intrusive::compare<ActivityNameComp> > ActivityNameTable;

	/* Comparator object for the Activity Id Table */
	struct ActivityIdComp {
		bool operator()(const Activity& act1, const Activity& act2) const;
		bool operator()(const activityId_t& id, const Activity& act) const;
		bool operator()(const Activity& act, const activityId_t& id) const;
	};

	typedef boost::intrusive::member_hook<Activity, Activity::ActivityTableItem,
		&Activity::m_idTableItem> ActivityIdTableOption;
	typedef boost::intrusive::multiset<Activity, ActivityIdTableOption,
		boost::intrusive::constant_time_size<false>,
		boost::intrusive::compare<ActivityIdComp> > ActivityIdTable;

	typedef boost::intrusive::member_hook<Activity, Activity::ActivityListItem,
		&Activity::m_runQueueItem> ActivityRunQueueOption;
	typedef boost::intrusive::list<Activity, ActivityRunQueueOption,
		boost::intrusive::constant_time_size<false> > ActivityRunQueue;

	typedef boost::intrusive::member_hook<Activity, Activity::ActivityListItem,
		&Activity::m_focusedListItem> ActivityFocusedListOption;
	typedef boost::intrusive::list<Activity, ActivityFocusedListOption,
		boost::intrusive::constant_time_size<false> > ActivityFocusedList;

	static const char *RunQueueNames[];

	/* Activity Name Table
	 * The most recent Activity to attempt to claim a name will be listed
	 * in the Table, and all Activties in the table are currently live and
	 * not ending.  Activities should be removed from the table as soon as
	 * they enter an ending state through cancel, stop, or complete (if it
	 * isn't going to restart the Activity). */
	ActivityNameTable	m_nameTable;

	/* Activity Id Table
	 * Tracks currently instantiated Activities by Id.  This is slgihtly
	 * different from the main Activity map, which tracks Activities that are
	 * live.  This includes Activities that are still in the process of
	 * being torn down.  (And will also enable automated tracking of Activities
	 * which were otherwise leaked). */
	ActivityIdTable		m_idTable;

	/* Activity Run Queues
	 * Start Queue: Activities may wait here if the Activity Manager isn't
	 * 	prepared to schedule the Activity (generally, just on startup)
	 * Ready Queue: Background Activities that are ready to run, but haven't
	 * 	gotten permission to do so yet
	 * Ready Interactive Queue: Ready Background Activities initiated by the
	 * 	user
	 * Background Run Queue: Background Activities that are currently running
	 * Background Interactive Run Queue: Background Interactive Activities
	 * 	that are currently running
	 * Immediate Run Queue: Immediate Activities that are currently running
	 */
	ActivityRunQueue	m_runQueue[RunQueueMax];

	/* Background Interactive Queue yield timeout */
	boost::shared_ptr<Timeout<ActivityManager> >	m_interactiveYieldTimeout;

	ActivityFocusedList	m_focusedActivities;

	ActivityMap		m_activities;

	unsigned		m_enabled;

	unsigned		m_backgroundConcurrencyLevel;
	unsigned		m_backgroundInteractiveConcurrencyLevel;

	unsigned		m_yieldTimeoutSeconds;

#ifndef ACTIVITYMANAGER_RANDOM_IDS
	activityId_t	m_nextActivityId;
#endif

	boost::shared_ptr<MasterResourceManager>	m_resourceManager;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_ACTIVITY_H__ */
