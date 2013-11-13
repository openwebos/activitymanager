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

#include "PowerActivity.h"
#include "Activity.h"
#include "Logging.h"

MojLogger PowerActivity::s_log(_T("activitymanager.poweractivity"));

PowerActivity::PowerActivity(boost::shared_ptr<PowerManager> manager,
	boost::shared_ptr<Activity> activity)
	: m_manager(manager)
	, m_activity(activity)
{
}

PowerActivity::~PowerActivity()
{
}

boost::shared_ptr<PowerManager> PowerActivity::GetManager()
{
	return m_manager;
}

boost::shared_ptr<Activity> PowerActivity::GetActivity()
{
	return m_activity.lock();
}

boost::shared_ptr<const Activity> PowerActivity::GetActivity() const
{
	return m_activity.lock();
}

NoopPowerActivity::NoopPowerActivity(boost::shared_ptr<PowerManager> manager,
	boost::shared_ptr<Activity> activity)
	: PowerActivity(manager, activity)
	, m_state(PowerUnlocked)
{
}

NoopPowerActivity::~NoopPowerActivity()
{
}

PowerActivity::PowerState NoopPowerActivity::GetPowerState() const
{
	return m_state;
}

void NoopPowerActivity::Begin()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] Locking power on",
		m_activity.lock()->GetId());

	if (m_state != PowerLocked) {
		m_state = PowerLocked;
		m_activity.lock()->PowerLockedNotification();
	}
}

void NoopPowerActivity::End()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] Unlocking power",
		m_activity.lock()->GetId());

	if (m_state != PowerUnlocked) {
		m_state = PowerUnlocked;
		m_activity.lock()->PowerUnlockedNotification();
	}
}

