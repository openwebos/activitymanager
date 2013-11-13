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

#include "Scheduler.h"
#include "Activity.h"
#include "Logging.h"
#include <stdexcept>
#include <cstdlib>

MojLogger Scheduler::s_log(_T("activitymanager.scheduler"));

Scheduler::Scheduler()
	: m_nextWakeup(0)
	, m_wakeScheduled(false)
	, m_localOffsetSet(false)
	, m_localOffset(0)
{
	/* Calculate a random base start time between 11pm and 5am so all
	 * the devices don't cause a storm of syncs if their midnights are
	 * aligned.  (Generally, local time should be used for that sort of thing,
	 * so at least they'll be distributed globally.  Also, technically,
	 * an hour spread would be ok, but there are longitudes with less
	 * subscribers, so spreading the more populated neighbors farther will
	 * still help.) */
#ifndef UNITTEST
	m_smartBase = (23*60*60) + (random() % (6*60*60));
#else
	m_smartBase = (24*60*60) + (60*60);
#endif
}

Scheduler::~Scheduler()
{
}

void Scheduler::AddItem(boost::shared_ptr<Schedule> item)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	LOG_DEBUG(
		"Adding [Activity %llu] to start at %llu (%s)",
		item->GetActivity()->GetId(),
		(unsigned long long)item->GetNextStartTime(),
		TimeToString(item->GetNextStartTime(), !item->IsLocal()).c_str());

	bool updateWake = false;

	if (item->IsLocal()) {
		m_localQueue.insert(*item);

		if (m_localQueue.iterator_to(*item) == m_localQueue.begin()) {
			updateWake = true;
		}
	} else {
		m_queue.insert(*item);

		if (m_queue.iterator_to(*item) == m_queue.begin()) {
			updateWake = true;
		}
	}

	if (updateWake) {
		DequeueAndUpdateTimeout();
	}
}

void Scheduler::RemoveItem(boost::shared_ptr<Schedule> item)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	LOG_DEBUG("Removing [Activity %llu]",
		item->GetActivity()->GetId());

	try {
		/* Do NOT attempt to get an iterator to an item that isn't in a
		 * container. */
		if (item->m_queueItem.is_linked()) {
			bool updateWake = false;

			/* If the item is at the head of either queue, the time might
			 * have changed.  Otherwise, it definitely didn't. */
			if (item->IsLocal()) {
				if (m_localQueue.iterator_to(*item) == m_localQueue.begin()) {
					updateWake = true;
				}
			} else {
				if (m_queue.iterator_to(*item) == m_queue.begin()) {
					updateWake = true;
				}
			}

			item->m_queueItem.unlink();

			if (updateWake) {
				DequeueAndUpdateTimeout();
			}
		}
	} catch (...) {
	}
}

std::string Scheduler::TimeToString(time_t convert, bool isUTC)
{
	char buf[32];
	struct tm tm;

	memset(&tm, 0, sizeof(struct tm));
	gmtime_r(&convert, &tm);

	size_t len = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
	if (isUTC) {
		buf[len] = 'Z';
		buf[len+1] = '\0';
	}

	return std::string(buf);
}

time_t Scheduler::StringToTime(const char *convert, bool& isUTC)
{
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));

	char *next = strptime(convert, "%Y-%m-%d %H:%M:%S", &tm);
	if (!next) {
		throw std::runtime_error("Failed to parse start time");
	}

	if (*next == 'Z') {
		isUTC = true;
	} else if (*next == '\0') {
		isUTC = false;
	} else {
		throw std::runtime_error("Start time must end in 'Z' for UTC, or "
			"nothing");
	}

	/* mktime performs conversions assuming the time is in the local timezone.
	 * timezone must be set for UTC for this to behave properly. */
	return mktime(&tm);
}

void Scheduler::SetLocalOffset(off_t offset)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Setting local offset to %lld", (long long)offset);

	bool updateWake = false;

	if (!m_localOffsetSet) {
		m_localOffset = offset;
		m_localOffsetSet = true;
		updateWake = true;
	} else if (m_localOffset != offset) {
		m_localOffset = offset;
		updateWake = true;
	}

	if (updateWake) {
		DequeueAndUpdateTimeout();
	}
}

off_t Scheduler::GetLocalOffset() const
{
	if (!m_localOffsetSet) {
		LOG_WARNING(MSGID_SCHE_OFFSET_NOTSET, 0, "Attempt to access local offset before it has been set");
	}

	return m_localOffset;
}

time_t Scheduler::GetSmartBaseTime() const
{
	return m_smartBase;
}

void Scheduler::Wake()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Wake callback");

	m_wakeScheduled = false;

	DequeueAndUpdateTimeout();
}

/* XXX Handle timer rollover? */
void Scheduler::DequeueAndUpdateTimeout()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	/* Nothing to do?  Then return.  A new timeout will be scheduled
	 * the next time something is queued */
	if (m_queue.empty() && (!m_localOffsetSet || m_localQueue.empty())) {
		LOG_DEBUG("Not dequeuing any items as queue is now empty");
		if (m_wakeScheduled) {
			CancelTimeout();
			m_wakeScheduled = false;
		}
		return;
	}

	time_t	curTime = time(NULL);

	LOG_DEBUG("Beginning to dequeue items at time %llu",
		(unsigned long long)curTime);

	/* If anything on the queue already happened in the past, dequeue it
	 * and mark it as Scheduled(). */

	ProcessQueue(m_queue, curTime);

	/* Only process the local queue if the timezone offset is known.
	 * Otherwise, wait, because it will be known shortly. */
	if (m_localOffsetSet) {
		ProcessQueue(m_localQueue, curTime + m_localOffset);
	}

	LOG_DEBUG("Done dequeuing items");

	/* Both queues scheduled and dequeued (or unknown if time zone is not
	 * yet known)? */
	if (m_queue.empty() && (!m_localOffsetSet || m_localQueue.empty())) {
		LOG_DEBUG("No unscheduled items remain");

		if (m_wakeScheduled) {
			CancelTimeout();
			m_wakeScheduled = false;
		}

		return;
	}

	time_t nextWakeup = GetNextStartTime();

	if (!m_wakeScheduled || (nextWakeup != m_nextWakeup)) {
		UpdateTimeout(nextWakeup, curTime);
		m_nextWakeup = nextWakeup;
		m_wakeScheduled = true;
	}
}

void Scheduler::ProcessQueue(ScheduleQueue& queue, time_t curTime)
{
	while (!queue.empty()) {
		Schedule& item = *(queue.begin());

		if (item.GetNextStartTime() <= curTime) {
			item.m_queueItem.unlink();
			item.Scheduled();
		} else {
			break;
		}
	}
}

void Scheduler::ReQueue(ScheduleQueue& queue)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Requeuing");

	ScheduleQueue updated;

	while (!queue.empty()) {
		Schedule& item = *(queue.begin());

		item.m_queueItem.unlink();
		item.CalcNextStartTime();
		updated.insert(item);
	}

	updated.swap(queue);
}

void Scheduler::TimeChanged()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("System time or timezone changed, recomputing start times and requeuing Scheduled Activities");

	ReQueue(m_queue);
	ReQueue(m_localQueue);

	DequeueAndUpdateTimeout();
}

time_t Scheduler::GetNextStartTime() const
{
	if (m_queue.empty()) {
		if (!m_localOffsetSet || m_localQueue.empty()) {
			throw std::runtime_error("No available items in queue");
		} else {
			const Schedule& localItem = *(m_localQueue.begin());
			return (localItem.GetNextStartTime() - m_localOffset);
		}
	} else {
		if (!m_localOffsetSet || m_localQueue.empty()) {
			const Schedule& item = *(m_queue.begin());
			return item.GetNextStartTime();
		} else {
			const Schedule& localItem = *(m_localQueue.begin());
			time_t nextLocalStartTime = (localItem.GetNextStartTime()
				- m_localOffset);

			const Schedule& item = *(m_queue.begin());
			time_t nextStartTime = item.GetNextStartTime();

			if (nextStartTime < nextLocalStartTime) {
				return nextStartTime;
			} else {
				return nextLocalStartTime;
			}
		}
	}
}

