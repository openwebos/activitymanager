// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "ContainerManager.h"
#include "ResourceContainer.h"
#include "BusId.h"
#include "BusEntity.h"

MojLogger ContainerManager::s_log(_T("activitymanager.resourcecontainermanager"));

ContainerManager::ContainerManager(
	boost::shared_ptr<MasterResourceManager> master)
	: m_master(master)
	, m_enabled(false)
{
}

ContainerManager::~ContainerManager()
{
}

boost::shared_ptr<ResourceContainer> ContainerManager::GetContainer(
	const std::string& name)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Looking up [Container %s]"), name.c_str());

	ContainerMap::iterator found = m_containers.find(name);
	if (found != m_containers.end()) {
		return found->second;
	}

	MojLogInfo(s_log, _T("Allocating new container for [Container %s]"),
		name.c_str());

	boost::shared_ptr<ResourceContainer> container = CreateContainer(name);
	m_containers[name] = container;

	return container;
}

/* This is tricky.  A software upgrade might move a service from one container
 * to another but not the other service names.  Don't fail in that case.
 * Keep a list of Bus Entities, and a list of Containers.  Move the Bus Entity
 * to the latest Mapping that was made.  Containers with no Entities are ok...
 * There might be processes still spawned in them, that we don't necessarily
 * want to kill, at least not right away.
 */
void ContainerManager::MapContainer(
	const std::string& name, const BusIdVec& ids, pid_t pid)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Mapping pid %d into [Container %s]"),
		(int)pid, name.c_str());

	/* Do not invalidate old Entities.  Just leave them in whatever container
	 * they were last in.  Move existing ones (and, of course, new ones) into
	 * whatever container is requested here */

	/* First, get the container. */
	boost::shared_ptr<ResourceContainer> container = GetContainer(name);

	/* Associate the bus entities */
	for (BusIdVec::const_iterator iter = ids.begin(); iter != ids.end();
		++iter) {
		boost::shared_ptr<BusEntity> entity = m_master.lock()->GetEntity(*iter);

		/* If it's currently mapped to a container, and it's a different
		 * container, unmap and remap.  Otherwise, if it's the same container,
		 * do nothing, or if no container, map. */
		EntityContainerMap::iterator eiter = m_entityContainers.find(entity);
		if (eiter != m_entityContainers.end()) {
			if (eiter->second != container) {
				/* Remove the entity from the current container, and
				 * update its priority as Activities may have been
				 * associated. */
				eiter->second->RemoveEntity(entity);
				eiter->second->UpdatePriority();

				container->AddEntity(entity);
				m_entityContainers[entity] = container;
			} /* else it's already in the correct container */
		} else {
			container->AddEntity(entity);
			m_entityContainers[entity] = container;
		}
	}

	/* Fix the priority of the container (the entities may already have
	 * existed, and may have live Activities */
	container->UpdatePriority();

	/* Now map the PID */
	container->MapProcess(pid);
}

void ContainerManager::InformEntityUpdated(boost::shared_ptr<BusEntity> entity)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[BusId %s] has been updated"),
		entity->GetName().c_str());

	EntityContainerMap::iterator citer = m_entityContainers.find(entity);
	if (citer == m_entityContainers.end()) {
		MojLogInfo(s_log, _T("No container currently mapped for "
			"[BusId %s]"), entity->GetName().c_str());
	} else {
		citer->second->UpdatePriority();
		MojLogInfo(s_log, _T("[BusId %s] priority is now \"%s\""),
			entity->GetName().c_str(),
			ActivityPriorityNames[citer->second->GetPriority()]);
	}
}

void ContainerManager::Enable()
{
	MojLogTrace(s_log);

	if (m_enabled) {
		MojLogNotice(s_log, _T("Container Manager already enabled"));
	}

	MojLogNotice(s_log, _T("Enabling Container Manager"));

	m_enabled = true;

	std::for_each(m_containers.begin(), m_containers.end(),
		boost::bind(&ResourceContainer::Enable,
			boost::bind(&ContainerMap::value_type::second, _1)));
}

void ContainerManager::Disable()
{
	MojLogTrace(s_log);

	if (!m_enabled) {
		MojLogNotice(s_log, _T("Container Manager already disabled"));
	}

	MojLogNotice(s_log, _T("Disabling Container Manager"));

	m_enabled = false;

	std::for_each(m_containers.begin(), m_containers.end(),
		boost::bind(&ResourceContainer::Disable,
			boost::bind(&ContainerMap::value_type::second, _1)));
}

bool ContainerManager::IsEnabled() const
{
	return m_enabled;
}

MojErr ContainerManager::InfoToJson(MojObject& rep) const
{
	MojObject containers(MojObject::TypeArray);

	std::for_each(m_containers.begin(), m_containers.end(),
		boost::bind(&ResourceContainer::PushJson,
			boost::bind(&ContainerMap::value_type::second, _1),
			boost::ref(containers)));

	MojErr err = rep.put(_T("containers"), containers);
	MojErrCheck(err);

	MojObject entityMap(MojObject::TypeArray);
	for (EntityContainerMap::const_iterator iter = m_entityContainers.begin();
		iter != m_entityContainers.end(); ++iter) {
		MojObject mapping(MojObject::TypeObject);

		MojString containerName;
		err = containerName.assign(iter->second->GetName().c_str());
		MojErrCheck(err);

		err = mapping.put(iter->first->GetName().c_str(), containerName);
		MojErrCheck(err);

		err = entityMap.push(mapping);
		MojErrCheck(err);
	}

	err = rep.put(_T("entityMap"), entityMap);
	MojErrCheck(err);

	return MojErrNone;
}

