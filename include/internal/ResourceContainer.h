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

#ifndef __ACTIVITYMANAGER_RESOURCECONTAINER_H__
#define __ACTIVITYMANAGER_RESOURCECONTAINER_H__

#include "Base.h"
#include "ActivityTypes.h"

#include <set>

class BusEntity;
class ContainerManager;

class ResourceContainer
{
public:
	ResourceContainer(const std::string& name,
		boost::shared_ptr<ContainerManager> manager);
	virtual ~ResourceContainer();

	void AddEntity(boost::shared_ptr<BusEntity> entity);
	void RemoveEntity(boost::shared_ptr<BusEntity> entity);

	const std::string& GetName() const;

	virtual void UpdatePriority() = 0;
	virtual void MapProcess(pid_t pid) = 0;

	ActivityPriority_t GetPriority() const;
	bool IsFocused() const;

	virtual void Enable() = 0;
	virtual void Disable() = 0;

	virtual MojErr ToJson(MojObject& rep) const;

	void PushJson(MojObject& array) const;

protected:
	static bool EntityPriorityCompare(const boost::shared_ptr<BusEntity>& x,
		const boost::shared_ptr<BusEntity>& y);

	typedef std::set<boost::shared_ptr<BusEntity> > EntitySet;

	/* A service might use multiple names on the bus */
	EntitySet	m_entities;

	std::string	m_name;

	boost::weak_ptr<ContainerManager>	m_manager;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_RESOURCECONTAINER_H__ */
