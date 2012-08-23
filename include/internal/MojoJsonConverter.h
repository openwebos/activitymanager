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

#ifndef __ACTIVITYMANAGER_MOJOJSONCONVERTER_H__
#define __ACTIVITYMANAGER_MOJOJSONCONVERTER_H__

#include "Base.h"
#include "Activity.h"

#include <core/MojObject.h>

class MojService;

class ActivityManager;
class MojoTriggerManager;
class Trigger;
class Callback;
class Scheduler;
class Schedule;
class RequirementManager;
class PowerManager;
class BusId;

class MojoJsonConverter {
public:
	MojoJsonConverter(MojService *service,
		boost::shared_ptr<ActivityManager> am,
		boost::shared_ptr<Scheduler> scheduler,
		boost::shared_ptr<MojoTriggerManager> triggerManager,
		boost::shared_ptr<RequirementManager> requirementManager,
		boost::shared_ptr<PowerManager> powerManager);
	virtual ~MojoJsonConverter();

	boost::shared_ptr<Activity> CreateActivity(const MojObject& spec,
		Activity::BusType bus, bool reload = false);

	void UpdateActivity(boost::shared_ptr<Activity> activity,
		const MojObject& spec);

	boost::shared_ptr<Activity> LookupActivity(const MojObject& payload,
		const BusId& caller);

	boost::shared_ptr<Trigger> CreateTrigger(
		boost::shared_ptr<Activity> activity, const MojObject& spec);

	boost::shared_ptr<Callback> CreateCallback(
		boost::shared_ptr<Activity> activity, const MojObject& spec);

	boost::shared_ptr<Schedule> CreateSchedule(
		boost::shared_ptr<Activity> activity, const MojObject& spec);

	static BusId ProcessBusId(const MojObject& spec);

protected:
	void ProcessTypeProperty(boost::shared_ptr<Activity> activity,
		const MojObject& type);

	typedef std::list<std::string> RequirementNameList;
	typedef std::list<boost::shared_ptr<Requirement> > RequirementList;

	void ProcessRequirements(boost::shared_ptr<Activity> activity,
		const MojObject& requirements, RequirementNameList& removedRequirements,
		RequirementList& addedRequirements);
	void UpdateRequirements(boost::shared_ptr<Activity> activity,
		RequirementNameList& removedRequirements,
		RequirementList& addedRequirements);

	MojService	*m_service;

	boost::shared_ptr<ActivityManager>		m_am;
	boost::shared_ptr<Scheduler>			m_scheduler;
	boost::shared_ptr<MojoTriggerManager>	m_triggerManager;
	boost::shared_ptr<RequirementManager>	m_requirementManager;
	boost::shared_ptr<PowerManager>			m_powerManager;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_MOJOJSONCONVERTER_H__ */
