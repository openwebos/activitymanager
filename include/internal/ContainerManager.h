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

#ifndef __ACTIVITYMANAGER_CONTAINERMANAGER_H__
#define __ACTIVITYMANAGER_CONTAINERMANAGER_H__

#include "ResourceManager.h"
#include "ActivityTypes.h"

#include <vector>
#include <map>

class ResourceContainer;
class BusEntity;

class ContainerManager : public ResourceManager
{
public:
	ContainerManager(boost::shared_ptr<MasterResourceManager> master);
	virtual ~ContainerManager();

	typedef std::vector<BusId> BusIdVec;

	virtual boost::shared_ptr<ResourceContainer> CreateContainer(
		const std::string& name) = 0;

	boost::shared_ptr<ResourceContainer> GetContainer(const std::string& name);

	void MapContainer(const std::string& name, const BusIdVec& ids, pid_t pid);

	virtual void InformEntityUpdated(boost::shared_ptr<BusEntity> entity);

	virtual ActivityPriority_t GetDefaultPriority() const = 0;
	virtual ActivityPriority_t GetDisabledPriority() const = 0;

	virtual void Enable();
	virtual void Disable();
	virtual bool IsEnabled() const;

	virtual MojErr InfoToJson(MojObject& rep) const;

protected:
	typedef std::map<boost::shared_ptr<BusEntity>,
		boost::shared_ptr<ResourceContainer> > EntityContainerMap;
	typedef std::map<std::string, boost::shared_ptr<ResourceContainer> >
		ContainerMap;

	EntityContainerMap	m_entityContainers;

	ContainerMap		m_containers;

	boost::weak_ptr<MasterResourceManager>	m_master;

	bool				m_enabled;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_CONTAINERMANAGER_H__ */
