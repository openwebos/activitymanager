/* @@@LICENSE
*
*      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#ifndef __ACTIVITYMANAGER_SCHEDULE_H__
#define __ACTIVITYMANAGER_SCHEDULE_H__

#include "Base.h"

#include <boost/intrusive/set.hpp>

class Activity;
class Scheduler;

class Schedule : public boost::enable_shared_from_this<Schedule>
{
public:
	Schedule(boost::shared_ptr<Scheduler> scheduler,
		boost::shared_ptr<Activity> activity, time_t start);
	virtual ~Schedule();

	virtual time_t GetNextStartTime() const;
	virtual void CalcNextStartTime();

	time_t GetTime() const;

	boost::shared_ptr<Activity> GetActivity() const;

	bool operator<(const Schedule& rhs) const;

	void Queue();
	void UnQueue();
	bool IsQueued() const;

	void Scheduled();
	bool IsScheduled() const;

	/* INTERFACE:  ACTIVITY ----> SCHEDULE */

	/* Activity has finished running, and all subscribers have unsubscribed. */
	virtual void InformActivityFinished();

	/* END INTERFACE */

	virtual bool ShouldReschedule() const;

	void SetLocal(bool local);
	bool IsLocal() const;

	virtual bool IsInterval() const;

	virtual MojErr ToJson(MojObject& rep, unsigned long flags) const;

	static const time_t DAY_ONE;
	static const time_t NEVER;
	static const time_t UNBOUNDED;

private:
	Schedule(const Schedule& copy);
	Schedule& operator=(const Schedule& copy);

protected:

	typedef boost::intrusive::set_member_hook<
		boost::intrusive::link_mode<
			boost::intrusive::auto_unlink> >	QueueItem;

	friend class Scheduler;

	QueueItem	m_queueItem;

	boost::shared_ptr<Scheduler>	m_scheduler;
	boost::weak_ptr<Activity>		m_activity;

	time_t	m_start;

	bool	m_local;

	bool	m_scheduled;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_SCHEDULE_H__ */
