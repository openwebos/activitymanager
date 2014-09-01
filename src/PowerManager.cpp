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

#include "PowerManager.h"
#include "PowerActivity.h"
#include "Activity.h"
#include "Logging.h"
#include <stdexcept>

MojLogger PowerManager::s_log(_T("activitymanager.powermanager"));

PowerManager::PowerManager()
{
}

PowerManager::~PowerManager()
{
}

void PowerManager::RequestBeginPowerActivity(
	boost::shared_ptr<Activity> activity)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Activity %lu requesting to begin power activity",
		(unsigned long)activity->GetId());

	boost::shared_ptr<PowerActivity> powerActivity =
		activity->GetPowerActivity();

	if (powerActivity->GetPowerState() != PowerActivity::PowerLocked) {
		m_poweredActivities.push_back(*powerActivity);
		powerActivity->Begin();
	} else {
		activity->PowerLockedNotification();
	}
}

void PowerManager::ConfirmPowerActivityBegin(
	boost::shared_ptr<Activity> activity)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Activity %lu confirmed power activity running",
		(unsigned long)activity->GetId());
}

void PowerManager::RequestEndPowerActivity(
	boost::shared_ptr<Activity> activity)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Activity %lu request to end power activity",
		(unsigned long)activity->GetId());

	boost::shared_ptr<PowerActivity> powerActivity =
		activity->GetPowerActivity();

	if (powerActivity->GetPowerState() != PowerActivity::PowerUnlocked) {
		powerActivity->End();
	} else {
		activity->PowerUnlockedNotification();
	}
}

void PowerManager::ConfirmPowerActivityEnd(
	boost::shared_ptr<Activity> activity)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Activity %lu confirmed power activity ended",
		(unsigned long)activity->GetId());

	boost::shared_ptr<PowerActivity> powerActivity =
		activity->GetPowerActivity();

	if (powerActivity->m_listItem.is_linked()) {
		powerActivity->m_listItem.unlink();
	}
}

MojErr PowerManager::InfoToJson(MojObject& rep) const
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);

	MojErr err;

	if (!m_poweredActivities.empty()) {
		MojObject poweredActivities(MojObject::TypeArray);

		std::for_each(m_poweredActivities.begin(), m_poweredActivities.end(),
			boost::bind(&Activity::PushIdentityJson,
				boost::bind<boost::shared_ptr<const Activity> >
					(&PowerActivity::GetActivity, _1),
				boost::ref(poweredActivities)));

		err = rep.put(_T("poweredActivities"), poweredActivities);
		MojErrCheck(err);
	}

	return MojErrNone;
}

NoopPowerManager::NoopPowerManager()
{
}

NoopPowerManager::~NoopPowerManager()
{
}

const std::string& NoopPowerManager::GetName() const
{
	static std::string name("NoopPowerManager");
	return name;
}

boost::shared_ptr<Requirement> NoopPowerManager::InstantiateRequirement(
	boost::shared_ptr<Activity> activity, const std::string& name,
	const MojObject& value)
{
	throw std::runtime_error("NoopPowerManager implements no requirements");
}

boost::shared_ptr<PowerActivity> NoopPowerManager::CreatePowerActivity(
	boost::shared_ptr<Activity> activity)
{
	return boost::make_shared<NoopPowerActivity>(
		boost::dynamic_pointer_cast<PowerManager, RequirementManager>
			(shared_from_this()), activity);
}

