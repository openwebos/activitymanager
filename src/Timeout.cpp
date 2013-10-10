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

#include "Timeout.h"

MojLogger TimeoutBase::s_log(_T("activitymanager.timeout"));

TimeoutBase::TimeoutBase(unsigned seconds)
	: m_seconds(seconds)
	, m_timeout(NULL)
{
}

TimeoutBase::~TimeoutBase()
{
	Cancel();
}

void TimeoutBase::Arm()
{
	MojLogTrace(s_log);

	if (m_timeout) {
		Cancel();
	}

	GSource *timeout = g_timeout_source_new_seconds((guint)m_seconds);
	g_source_set_callback(timeout, TimeoutBase::StaticWakeupTimeout, this,
		NULL);
	g_source_attach(timeout, g_main_context_default());

	m_timeout = timeout;
}

void TimeoutBase::Cancel()
{
	MojLogTrace(s_log);

	if (m_timeout) {
		if (!g_source_is_destroyed(m_timeout)) {
			g_source_destroy(m_timeout);
		}

		m_timeout = NULL;
	}
}

gboolean TimeoutBase::StaticWakeupTimeout(gpointer data)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Wakeup"));

	TimeoutBase *base = static_cast<TimeoutBase *>(data);

	/* Reset NOW, because the timeout call could delete this object. */
	base->m_timeout = NULL;

	try {
		base->WakeupTimeout();
	} catch (const std::exception& except) {
		MojLogError(s_log, _T("Unhandled exception \"%s\" occurred"),
			except.what());
	} catch (...) {
		MojLogError(s_log, _T("Unhandled exception of unknown type occurred"));
	}

	return false;
}

