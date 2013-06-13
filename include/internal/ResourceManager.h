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

#ifndef __ACTIVITYMANAGER_RESOURCEMANAGER_H__
#define __ACTIVITYMANAGER_RESOURCEMANAGER_H__

#include "Base.h"

#include <map>

class Activity;
class BusId;
class BusEntity;

class ResourceManager : public boost::enable_shared_from_this<ResourceManager>
{
public:
	ResourceManager();
	virtual ~ResourceManager();

	virtual void InformEntityUpdated(boost::shared_ptr<BusEntity> entity) = 0;

	virtual void Enable() = 0;
	virtual void Disable() = 0;
	virtual bool IsEnabled() const = 0;

	virtual MojErr InfoToJson(MojObject& rep) const = 0;
};

/* Different resources might have different managers, and the mapping of
 * entities to containers might also be different for different managers.
 * The groupings for CPU, for example, might be very fine grained, whereas
 * memory less so, and storage IO even less. */
class MasterResourceManager : public ResourceManager
{
public:
	MasterResourceManager();
	virtual ~MasterResourceManager();

	void Associate(boost::shared_ptr<Activity> activity);
	void Associate(boost::shared_ptr<Activity> activity, const BusId& id);
	void Dissociate(boost::shared_ptr<Activity> activity);
	void Dissociate(boost::shared_ptr<Activity> activity, const BusId& id);

	void UpdateAssociations(boost::shared_ptr<Activity> activity);

	void SetManager(const std::string& resource,
		boost::shared_ptr<ResourceManager> manager);

	boost::shared_ptr<BusEntity> GetEntity(const BusId& id);

	virtual void InformEntityUpdated(boost::shared_ptr<BusEntity> entity);

	virtual void Enable();
	virtual void Disable();
	virtual bool IsEnabled() const;

	virtual MojErr InfoToJson(MojObject& rep) const;

protected:
	typedef std::map<std::string, boost::shared_ptr<ResourceManager> >
		ResourceManagerMap;
	typedef std::map<BusId, boost::shared_ptr<BusEntity> > EntityMap;

	ResourceManagerMap	m_managers;
	EntityMap			m_entities;

	bool	m_enabled;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_RESOURCEMANAGER_H__ */
