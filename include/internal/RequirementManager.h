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

#ifndef __ACTIVITYMANAGER_REQUIREMENTMANAGER_H__
#define __ACTIVITYMANAGER_REQUIREMENTMANAGER_H__

#include "Base.h"
#include "Requirement.h"

#include <set>
#include <map>

class Activity;
class MasterRequirementManager;

class RequirementManager :
	public boost::enable_shared_from_this<RequirementManager>
{
public:
	RequirementManager();
	virtual ~RequirementManager();

	virtual const std::string& GetName() const = 0;

	virtual boost::shared_ptr<Requirement> InstantiateRequirement(
		boost::shared_ptr<Activity> activity, const std::string& name,
		const MojObject& value) = 0;

	/* Register all requirement instantiation functions with given master. */
	virtual void RegisterRequirements(
		boost::shared_ptr<MasterRequirementManager> master);
	virtual void UnregisterRequirements(
		boost::shared_ptr<MasterRequirementManager> master);

	virtual void Enable();
	virtual void Disable();

protected:
	typedef boost::intrusive::member_hook<ListedRequirement,
		ListedRequirement::RequirementListItem,
		&ListedRequirement::m_managerListItem> RequirementListOption;
	typedef boost::intrusive::list<ListedRequirement, RequirementListOption,
		boost::intrusive::constant_time_size<false> > RequirementList;
};

class MasterRequirementManager : public RequirementManager
{
public:
	MasterRequirementManager();
	virtual ~MasterRequirementManager();

	virtual const std::string& GetName() const;

	boost::shared_ptr<Requirement> InstantiateRequirement(
		boost::shared_ptr<Activity> activity, const std::string& name,
		const MojObject& value);

	void RegisterRequirement(const std::string& name,
		boost::shared_ptr<RequirementManager> manager);
	void UnregisterRequirement(const std::string& name,
		boost::shared_ptr<RequirementManager> manager);

	virtual void Enable();
	virtual void Disable();

	virtual void AddManager(boost::shared_ptr<RequirementManager> manager);
	virtual void RemoveManager(boost::shared_ptr<RequirementManager> manager);

protected:
	typedef std::set<boost::shared_ptr<RequirementManager> > ManagerSet;
	typedef std::map<std::string, boost::shared_ptr<RequirementManager> >
		RequirementMap;

	ManagerSet		m_managers;
	RequirementMap	m_requirements;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_REQUIREMENTMANAGER_H__ */
