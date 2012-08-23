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

#ifndef __ACTIVITYMANAGER_TIMEOUT_H__
#define __ACTIVITYMANAGER_TIMEOUT_H__

#include "Base.h"
#include "glib.h"

class TimeoutBase
{
public:
	TimeoutBase(unsigned seconds);
	virtual ~TimeoutBase();

	void Arm();
	void Cancel();

protected:
	virtual void WakeupTimeout() = 0;
	static gboolean StaticWakeupTimeout(gpointer data);

	unsigned	m_seconds;
	GSource		*m_timeout;

	static MojLogger	s_log;
};

template <class T>
class Timeout : public TimeoutBase
{
public:
	typedef void (T::* CallbackType)();

	Timeout(boost::shared_ptr<T> target, unsigned seconds,
		CallbackType callback)
		: TimeoutBase(seconds)
		, m_callback(callback)
		, m_target(target)
	{
	}

	virtual ~Timeout() {}

	virtual void WakeupTimeout()
	{
		if (!m_target.expired()) {
			((*m_target.lock().get()).*m_callback)();
		}
	}

protected:
	CallbackType		m_callback;
	boost::weak_ptr<T>	m_target;
};

#endif /* __ACTIVITYMANAGER_TIMEOUT_H__ */
