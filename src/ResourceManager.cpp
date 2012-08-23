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

#include "ResourceManager.h"
#include "Activity.h"
#include "BusEntity.h"

#include <stdexcept>

MojLogger MasterResourceManager::s_log(_T("activitymanager.resourcemanager"));

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

MasterResourceManager::MasterResourceManager()
	: m_enabled(true)
{
}

MasterResourceManager::~MasterResourceManager()
{
}

void MasterResourceManager::Associate(boost::shared_ptr<Activity> activity)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Associating all subscribers of [Activity %llu]"),
		activity->GetId());

	/* Place all processes associated with the unique subscribers of the
	 * Activity under dynamic priority control. */
	Activity::SubscriberIdVec subscribers = activity->GetUniqueSubscribers();

	for (Activity::SubscriberIdVec::iterator iter = subscribers.begin();
		iter != subscribers.end(); ++iter) {
		Associate(activity, *iter);
	}
}

void MasterResourceManager::Associate(boost::shared_ptr<Activity> activity,
	const BusId& id)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Associating [Activity %llu] with [BusId %s] "
		"on all Managers"), activity->GetId(), id.GetString().c_str());

	/* Look up Bus Entity, and Associate the Activity. */
	boost::shared_ptr<BusEntity> entity = GetEntity(id);
	entity->AssociateActivity(activity);
	InformEntityUpdated(entity);
}

void MasterResourceManager::Dissociate(boost::shared_ptr<Activity> activity)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Dissociating all subscribers of [Activity %llu]"),
		activity->GetId());

	/* Remove associations between the Activity and any remaining unique
	 * subscribers. */
	Activity::SubscriberIdVec subscribers = activity->GetUniqueSubscribers();

	for (Activity::SubscriberIdVec::iterator iter = subscribers.begin();
		iter != subscribers.end(); ++iter) {
		Dissociate(activity, *iter);
	}
}

void MasterResourceManager::Dissociate(boost::shared_ptr<Activity> activity, const BusId& id)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Dissociating [Activity %llu] from [BusId %s] on all "
		"Managers"), activity->GetId(), id.GetString().c_str());

	/* Look up the Bus Entity, and Dissociate the Activity. */
	boost::shared_ptr<BusEntity> entity = GetEntity(id);
	entity->DissociateActivity(activity);
	InformEntityUpdated(entity);
}

void MasterResourceManager::UpdateAssociations(
	boost::shared_ptr<Activity> activity)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Updating all unique entities associated with "
		"[Activity %llu]"), activity->GetId());

	Activity::SubscriberIdVec subscribers = activity->GetUniqueSubscribers();

	for (Activity::SubscriberIdVec::iterator iter = subscribers.begin();
		iter != subscribers.end(); ++iter) {
		InformEntityUpdated(GetEntity(*iter));
	}
}

void MasterResourceManager::SetManager(const std::string& resource,
	boost::shared_ptr<ResourceManager> manager)
{
	MojLogTrace(s_log);
	MojLogNotice(s_log, _T("Assigning Resource Manager for resource \"%s\""),
		resource.c_str());

	ResourceManagerMap::iterator found = m_managers.find(resource);
	if (found != m_managers.end()) {
		MojLogError(s_log, _T("Resource Manager for resource \"%s\" has "
			"already been set"), resource.c_str());
		throw std::runtime_error("Resource Manager already set for resource");
	}

	m_managers[resource] = manager;

	/* Make sure the child Resource Manager matches the enable state of the
 	 * Master */
	if (m_enabled) {
		if (!manager->IsEnabled()) {
			MojLogNotice(s_log, _T("Enabling Resource Manager for "
				"resource %s"), resource.c_str());
			manager->Enable();
		}
	} else {
		if (manager->IsEnabled()) {
			MojLogNotice(s_log, _T("Disabling Resource Manager for "
				"resource %s"), resource.c_str());
			manager->Disable();
		}
	}
}

boost::shared_ptr<BusEntity> MasterResourceManager::GetEntity(const BusId& id)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Looking up entity for [BusId %s]"),
		id.GetString().c_str());

	EntityMap::iterator found = m_entities.find(id);
	if (found != m_entities.end()) {
		return found->second;
	}

	MojLogInfo(s_log, _T("Allocating new entity for [BusId %s]"),
		id.GetString().c_str());

	boost::shared_ptr<BusEntity> entity = boost::make_shared<BusEntity>(id);
	m_entities[id] = entity;

	return entity;
}

void MasterResourceManager::InformEntityUpdated(
	boost::shared_ptr<BusEntity> entity)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Informing all managers that [BusId %s] has been "
		"updated"), entity->GetName().c_str());

	for (ResourceManagerMap::iterator iter = m_managers.begin();
		iter != m_managers.end(); ++iter) {
		iter->second->InformEntityUpdated(entity);
	}
}
void MasterResourceManager::Enable()
{
	MojLogTrace(s_log);
	MojLogNotice(s_log, _T("Enabling all Resource Managers"));

	for (ResourceManagerMap::iterator iter = m_managers.begin();
		iter != m_managers.end(); ++iter) {
		MojLogNotice(s_log, _T("Enabling Resource Manager for resource %s"),
			iter->first.c_str());
		iter->second->Enable();
	}

	m_enabled = true;
}

void MasterResourceManager::Disable()
{
	MojLogTrace(s_log);
	MojLogNotice(s_log, _T("Disabling all Resource Managers"));

	for (ResourceManagerMap::iterator iter = m_managers.begin();
		iter != m_managers.end(); ++iter) {
		MojLogNotice(s_log, _T("Disabling Resource Manager for resource %s"),
			iter->first.c_str());
		iter->second->Disable();
	}

	m_enabled = false;
}

bool MasterResourceManager::IsEnabled() const
{
	return m_enabled;
}

MojErr MasterResourceManager::InfoToJson(MojObject& rep) const
{
	MojObject resourceManager(MojObject::TypeObject);

	MojObject entities(MojObject::TypeArray);

	std::for_each(m_entities.begin(), m_entities.end(),
		boost::bind(&BusEntity::PushJson,
			boost::bind(&EntityMap::value_type::second, _1),
			boost::ref(entities), true));

	MojErr err = resourceManager.put(_T("entities"), entities);
	MojErrCheck(err);

	MojObject resources(MojObject::TypeObject);

	for (ResourceManagerMap::const_iterator iter = m_managers.begin();
		iter != m_managers.end(); ++iter) {
		MojObject managerRep(MojObject::TypeObject);

		err = iter->second->InfoToJson(managerRep);
		MojErrCheck(err);

		err = resources.put(iter->first.c_str(), managerRep);
		MojErrCheck(err);
	}

	err = resourceManager.put(_T("resources"), resources);
	MojErrCheck(err);

	err = rep.put(_T("resourceManager"), resourceManager);
	MojErrCheck(err);

	return MojErrNone;
}

