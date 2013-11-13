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

#include "IntervalSchedule.h"
#include "Scheduler.h"
#include "Activity.h"
#include "ActivityJson.h"
#include "Logging.h"

#include <boost/regex.hpp>
#include <sstream>

#include <cstdlib>

IntervalSchedule::IntervalSchedule(boost::shared_ptr<Scheduler> scheduler,
	boost::shared_ptr<Activity> activity, time_t start, unsigned interval,
	 time_t end)
	: Schedule(scheduler, activity, start)
	, m_end(end)
	, m_interval(interval)
	, m_skip(false)
	, m_lastFinished(NEVER)
{
}

IntervalSchedule::~IntervalSchedule()
{
}

time_t IntervalSchedule::GetNextStartTime() const
{
	return m_nextStart;
}

/*
 * Default interval scheduling policy is to align the scheduling points at
 * each of the interval tick points from the start point of the schedule.
 *
 * If missed is set, and a tick or ticks was missed, the Scheduler will run
 * the Activity *once* to catch up, and then continue to run at the aligned
 * points. */
void IntervalSchedule::CalcNextStartTime()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	time_t curTime = GetTime();
	time_t start = GetBaseStartTime();

	/* Find the next upcoming scheduling point */
	if (curTime > start) {
		time_t next = curTime - start;
		next /= m_interval;
		next += 1;

		m_nextStart = start + (next * m_interval);
	} else {
		m_nextStart = start;
	}

	/* If the schedule slipped, make it available to run immediately, this
	 * time. */
	if (!m_skip) {
		/* Last finished could be in the future if there was a time zone
		 * change.  If so, just ignore it.  (Below will be < 0 and not
		 * trip) */
		if (m_lastFinished == NEVER) {
			if ((m_start != DAY_ONE) &&
				((unsigned)(m_nextStart - m_start) > m_interval)) {
				m_nextStart = curTime;
			}
		} else if (((m_nextStart - m_lastFinished) > 0) &&
			((unsigned)(m_nextStart - m_lastFinished) > m_interval)) {
			m_nextStart = curTime;
		}
	}

	LOG_DEBUG("[Activity %lu] Next start time is %s",
		(unsigned long)m_activity.lock()->GetId(),
		Scheduler::TimeToString(m_nextStart, !m_local).c_str());
}

time_t IntervalSchedule::GetBaseStartTime() const
{
	return m_scheduler->GetSmartBaseTime();
}

bool IntervalSchedule::IsInterval() const
{
	return true;
}

void IntervalSchedule::SetSkip(bool skip)
{
	m_skip = skip;
}

void IntervalSchedule::SetLastFinishedTime(time_t finished)
{
	/* Only finished times in the past, and after when the
	 * event is supposed to have started running, are accepted. */
	if (((GetTime() - finished) > 0) && ((finished - m_start) > 0)) {
		m_lastFinished = finished;
	}
}

void IntervalSchedule::InformActivityFinished()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	time_t lastFinished = GetTime();

	LOG_DEBUG("[Activity %lu] Finished at %llu",
		(unsigned long)m_activity.lock()->GetId(),
		(unsigned long long)lastFinished);

	/* Because the timezone can change to move time backwards, it's possible
	 * for the current time to move *before* the scheduled time if this
	 * happens while the event is running.  Don't let that happen, but
	 * otherwise let it move where it wants.  It's for protection or further
	 * relative scheduling so we want it to be as accurate as possible for
	 * the current time on the device */
	if ((lastFinished - m_start) > 0) {
		m_lastFinished = lastFinished;
	}
}

bool IntervalSchedule::ShouldReschedule() const
{
	if (m_end == UNBOUNDED) {
		return true;
	} else {
		return ((unsigned)(m_end - m_nextStart) > m_interval);
	}
}

MojErr IntervalSchedule::ToJson(MojObject& rep, unsigned long flags) const
{
	MojErr err;

	if (flags & ACTIVITY_JSON_CURRENT) {
		err = rep.putString(_T("nextStart"),
			Scheduler::TimeToString(m_nextStart, !m_local).c_str());
		MojErrCheck(err);
	}

	if (m_end != UNBOUNDED) {
		err = rep.putString(_T("end"),
			Scheduler::TimeToString(m_end, !m_local).c_str());
		MojErrCheck(err);
	}

	if (m_skip) {
		err = rep.putBool(_T("skip"), true);
		MojErrCheck(err);
	}

	if (m_lastFinished != NEVER) {
		err = rep.putString(_T("lastFinished"),
			Scheduler::TimeToString(m_lastFinished, !m_local).c_str());
		MojErrCheck(err);
	}

	err = rep.putString(_T("interval"), IntervalToString(m_interval).c_str());
	MojErrCheck(err);

	return Schedule::ToJson(rep, flags);
}

unsigned IntervalSchedule::StringToInterval(const char *intervalStr,
	bool smart)
{
	static const boost::regex durationRegex(
		"(?:(\\d+)D)?(?:(\\d+)H)?(?:(\\d+)M)?(?:(\\d+)S)?",
		boost::regex::icase | boost::regex::optimize);

	boost::cmatch what;

	if (boost::regex_match(intervalStr, what, durationRegex)) {
		int days = 0, hours = 0, minutes = 0, seconds = 0;

		if (what[1].length()) {
			days = atoi(what[1].first);
		}
		if (what[2].length()) {
			hours = atoi(what[2].first);
		}
		if (what[3].length()) {
			minutes = atoi(what[3].first);
		}
		if (what[4].length()) {
			seconds = atoi(what[4].first);
		}

		unsigned totalSecs = seconds + (minutes * 60) + (hours * 3600) +
			(days * 86400);

		if (totalSecs == 0) {
			throw std::runtime_error("Duration must be non-zero");
		}

		if (!smart) {
			return totalSecs;
		}

		if ((totalSecs % 60) != 0) {
			throw std::runtime_error("Only durations of even minutes may be "
				"specified");
		}

		unsigned total = totalSecs/60;

		static const unsigned allowed[] =
			{ 5, 10, 15, 20, 30, 60, 180, 360, 720 };
		static const int num_allowed = (sizeof(allowed)/sizeof(int));

		int i;
		for (i = 0; i < num_allowed; i++) {
			if (total == allowed[i])
				break;
		}

		if (i < num_allowed) {
			/* Explicitly in the allowed list */
			return totalSecs;
		} else if ((total % 1440) == 0) {
			/* Intervals in whole days are also allowed */
			return totalSecs;
		} else {
			throw std::runtime_error("Interval must be a number of days"
				"<n>d, or one of: 12h, 6h, 3h, 1h, 20m, 30m, 15m, 10m or 5m");
		}
	} else {
		throw std::runtime_error("Failed to parse scheduling interval");
	}
}

std::string IntervalSchedule::IntervalToString(unsigned interval)
{
	int seconds = interval % 60;
	interval /= 60;
	int minutes = interval % 60;
	interval /= 60;
	int hours = interval % 24;
	interval /= 24;

	std::stringstream intervalStr;
	if (interval) {
		intervalStr << interval << "d";
	}

	if (hours) {
		intervalStr << hours << "h";
	}

	if (minutes) {
		intervalStr << minutes << "m";
	}

	if (seconds) {
		intervalStr << seconds << "s";
	}

	return intervalStr.str();
}

PreciseIntervalSchedule::PreciseIntervalSchedule(
	boost::shared_ptr<Scheduler> scheduler,
	boost::shared_ptr<Activity> activity, time_t start, unsigned interval,
	time_t end)
	: IntervalSchedule(scheduler, activity, start, interval, end)
{
}

PreciseIntervalSchedule::~PreciseIntervalSchedule()
{
}

time_t PreciseIntervalSchedule::GetBaseStartTime() const
{
	return m_start;
}

MojErr PreciseIntervalSchedule::ToJson(MojObject& rep,
	unsigned long flags) const
{
	MojErr err = rep.putBool(_T("precise"), true);
	MojErrCheck(err);

	return IntervalSchedule::ToJson(rep, flags);
}

RelativeIntervalSchedule::RelativeIntervalSchedule(
	boost::shared_ptr<Scheduler> scheduler,
	boost::shared_ptr<Activity> activity, time_t start, unsigned interval,
	time_t end)
	: IntervalSchedule(scheduler, activity, start, interval, end)
{
}

RelativeIntervalSchedule::~RelativeIntervalSchedule()
{
}

time_t RelativeIntervalSchedule::GetBaseStartTime() const
{
	if (m_lastFinished != NEVER) {
		return m_lastFinished;
	} else {
		return m_start;
	}
}

MojErr RelativeIntervalSchedule::ToJson(MojObject& rep,
	unsigned long flags) const
{
	MojErr err;

	err = rep.putBool(_T("precise"), true);
	MojErrCheck(err);

	err = rep.putBool(_T("relative"), true);
	MojErrCheck(err);

	return IntervalSchedule::ToJson(rep, flags);
}

