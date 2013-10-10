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

#include "GlibScheduler.h"

GlibScheduler::GlibScheduler()
{
}

GlibScheduler::~GlibScheduler()
{
}

void GlibScheduler::Enable()
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Enabling scheduler"));
}

void GlibScheduler::UpdateTimeout(time_t nextWakeup, time_t curTime)
{
	MojLogTrace(s_log);

	MojLogDebug(s_log, _T("Updating wakeup timer: next wakeup %llu, "
		"current time %llu"), (unsigned long long)nextWakeup,
		(unsigned long long)curTime);

	m_timeout = boost::make_shared<Timeout<GlibScheduler> >(
		boost::dynamic_pointer_cast<GlibScheduler, Scheduler>(
			shared_from_this()), (int)(nextWakeup - curTime),
		&GlibScheduler::TimeoutCallback);
	m_timeout->Arm();
}

void GlibScheduler::CancelTimeout()
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Scheduler timeout cancelled"));

	if (m_timeout) {
		m_timeout.reset();
	}
}

void GlibScheduler::TimeoutCallback()
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Scheduler timeout callback"));

	m_timeout.reset();

	Wake();
}

