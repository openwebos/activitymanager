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

#ifndef __ACTIVITYMANAGER_SCHEDULER_H__
#define __ACTIVITYMANAGER_SCHEDULER_H__

#include "Base.h"
#include "Schedule.h"

class Scheduler : public boost::enable_shared_from_this<Scheduler>
{
public:
	Scheduler();
	virtual ~Scheduler();

	void AddItem(boost::shared_ptr<Schedule> item);
	void RemoveItem(boost::shared_ptr<Schedule> item);

	static std::string TimeToString(time_t convert, bool isUTC);
	static time_t StringToTime(const char *convert, bool& isUTC);

	void SetLocalOffset(off_t offset);
	off_t GetLocalOffset() const;

	time_t GetSmartBaseTime() const;

	virtual void Enable() = 0;

protected:
	virtual void UpdateTimeout(time_t nextWakeup, time_t curTime) = 0;
	virtual void CancelTimeout() = 0;

	typedef boost::intrusive::member_hook<Schedule,
		Schedule::QueueItem, &Schedule::m_queueItem> ScheduleQueueOption;
	typedef boost::intrusive::multiset<Schedule, ScheduleQueueOption,
		boost::intrusive::constant_time_size<false> > ScheduleQueue;

	void Wake();
	void DequeueAndUpdateTimeout();
	void ProcessQueue(ScheduleQueue& queue, time_t curTime);
	void ReQueue(ScheduleQueue& queue);

	void TimeChanged();

	time_t GetNextStartTime() const;

	/* Priority queues of tasks, in order, sorted by next run time.
	 * Two queues, one in absolute time, and one in local time. */
	ScheduleQueue	m_queue;
	ScheduleQueue	m_localQueue;

	time_t			m_nextWakeup;
	bool			m_wakeScheduled;

	bool			m_localOffsetSet;
	off_t			m_localOffset;

	time_t			m_smartBase;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_SCHEDULER_H__ */
