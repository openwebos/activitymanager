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

#include "Schedule.h"
#include "Scheduler.h"
#include "Activity.h"
#include "ActivityJson.h"

#include <ctime>

const time_t Schedule::DAY_ONE = (60*60*24);
const time_t Schedule::UNBOUNDED = -1;
const time_t Schedule::NEVER = -1;

MojLogger Schedule::s_log(_T("activitymanager.schedule"));

Schedule::Schedule(boost::shared_ptr<Scheduler> scheduler,
	boost::shared_ptr<Activity> activity, time_t start)
	: m_scheduler(scheduler)
	, m_activity(activity)
	, m_start(start)
	, m_local(false)
	, m_scheduled(false)
{
}

Schedule::~Schedule()
{
}

time_t Schedule::GetNextStartTime() const
{
	return m_start;
}

void Schedule::CalcNextStartTime()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] Not an interval schedule, so next "
		"start time not updated"), m_activity.lock()->GetId());
}

time_t Schedule::GetTime() const
{
	time_t curTime = time(NULL);

	/* Adjust back to local time, if required.  This may not be the same
	 * adjustment that was in place when the scheduled Activity was first
	 * scheduled, because the time zone may have changed while it was running,
	 * or after it was scheduled but before it made it through the queue to
	 * run.  Should be close enough though.  Generally the time does not
	 * change... */

	if (m_local) {
		curTime += m_scheduler->GetLocalOffset();
	}

	return curTime;
}

boost::shared_ptr<Activity> Schedule::GetActivity() const
{
	return m_activity.lock();
}

bool Schedule::operator<(const Schedule& rhs) const
{
	return GetNextStartTime() < rhs.GetNextStartTime();
}

void Schedule::Queue()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] Queueing with Scheduler"),
		m_activity.lock()->GetId());

	if (IsQueued())
		UnQueue();

	CalcNextStartTime();

	m_scheduled = false;
	m_scheduler->AddItem(shared_from_this());
}

void Schedule::UnQueue()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] Unqueueing from Scheduler"),
		m_activity.lock()->GetId());

	m_scheduler->RemoveItem(shared_from_this());
	m_scheduled = false;
}

bool Schedule::IsQueued() const
{
	return m_queueItem.is_linked();
}

void Schedule::Scheduled()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] Scheduled"),
		m_activity.lock()->GetId());

	m_scheduled = true;
	m_activity.lock()->Scheduled();
}

bool Schedule::IsScheduled() const
{
	return m_scheduled;
}

void Schedule::InformActivityFinished()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] Finished"),
		m_activity.lock()->GetId());
}

bool Schedule::ShouldReschedule() const
{
	return false;
}

void Schedule::SetLocal(bool local)
{
	m_local = local;
}

bool Schedule::IsLocal() const
{
	return m_local;
}

bool Schedule::IsInterval() const
{
	return false;
}

MojErr Schedule::ToJson(MojObject& rep, unsigned long flags) const
{
	MojErr err;

	if (flags & ACTIVITY_JSON_CURRENT) {
		err = rep.putBool(_T("scheduled"), m_scheduled);
		MojErrCheck(err);
	}

	if (m_local) {
		err = rep.putBool(_T("local"), m_local);
		MojErrCheck(err);
	}

	if (m_start != DAY_ONE) {
		err = rep.putString(_T("start"),
			Scheduler::TimeToString(m_start, !m_local).c_str());
		MojErrCheck(err);
	}

	return MojErrNone;
}

