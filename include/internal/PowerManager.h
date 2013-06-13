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

#ifndef __ACTIVITYMANAGER_POWERMANAGER_H__
#define __ACTIVITYMANAGER_POWERMANAGER_H__

#include "Base.h"
#include "RequirementManager.h"
#include "PowerActivity.h"

/* PowerManager is responsible for the "battery" and "charging" requirements,
 * in addition to the power activity management APIs */

class PowerActivity;

class PowerManager : public RequirementManager
{
public:
	PowerManager();
	virtual ~PowerManager();

	virtual boost::shared_ptr<PowerActivity> CreatePowerActivity(
		boost::shared_ptr<Activity> activity) = 0;

	/* The Activity will request permission to begin or end, which will set
	 * into motion a chain of events that will ultimately call its being
	 * informed that power has been locked or unlocked, at which point it
	 * will confirm to the Power Manager this is complete.
	 *
	 * Control flows from the Activity -> Power Manager -> Power Activity,
	 * then from the Power Activity -> Activity -> Power Manager.
	 */
	virtual void RequestBeginPowerActivity(
		boost::shared_ptr<Activity> activity);
	virtual void RequestEndPowerActivity(
		boost::shared_ptr<Activity> activity);
	virtual void ConfirmPowerActivityBegin(
		boost::shared_ptr<Activity> activity);
	virtual void ConfirmPowerActivityEnd(
		boost::shared_ptr<Activity> activity);

	MojErr InfoToJson(MojObject& rep) const;

protected:
	typedef boost::intrusive::member_hook<PowerActivity,
		PowerActivity::ListItem, &PowerActivity::m_listItem>
		PowerActivityListOption;
	typedef boost::intrusive::list<PowerActivity, PowerActivityListOption,
		boost::intrusive::constant_time_size<false> > PowerActivityList;

	PowerActivityList	m_poweredActivities;

	static MojLogger	s_log;
};

class NoopPowerManager : public PowerManager
{
public:
	NoopPowerManager();
	virtual ~NoopPowerManager();

	/* XXX Separate RequirementManager and PowerManager */
	virtual const std::string& GetName() const;
	virtual boost::shared_ptr<Requirement> InstantiateRequirement(
		boost::shared_ptr<Activity> activity, const std::string& name,
		const MojObject& value);

	virtual boost::shared_ptr<PowerActivity> CreatePowerActivity(
		boost::shared_ptr<Activity> activity);
};

#endif /* __ACTIVITYMANAGER_POWERMANAGER_H__ */
