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

#include "ResourceContainer.h"
#include "ContainerManager.h"
#include "BusEntity.h"

#include <stdexcept>

MojLogger ResourceContainer::s_log(_T("activitymanager.resourcecontainer"));

ResourceContainer::ResourceContainer(const std::string& name,
	boost::shared_ptr<ContainerManager> manager)
	: m_name(name)
	, m_manager(manager)
{
}

ResourceContainer::~ResourceContainer()
{
}

void ResourceContainer::AddEntity(boost::shared_ptr<BusEntity> entity)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Adding [BusId %s] to [Container %s]"),
		entity->GetName().c_str(), m_name.c_str());

	EntitySet::iterator found = m_entities.find(entity);
	if (found != m_entities.end()) {
		MojLogWarning(s_log, _T("[BusId %s] has already been added to "
			"[Container %s]"), entity->GetName().c_str(), m_name.c_str());
		return;
	}

	m_entities.insert(entity);
}

void ResourceContainer::RemoveEntity(boost::shared_ptr<BusEntity> entity)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Removing [BusId %s] from [Container %s]"),
		entity->GetName().c_str(), m_name.c_str());

	EntitySet::iterator found = m_entities.find(entity);
	if (found == m_entities.end()) {
		MojLogWarning(s_log, _T("[BusId %s] is not currently mapped to "
			"[Container %s]"), entity->GetName().c_str(), m_name.c_str());
		return;
	}

	m_entities.erase(found);
}

const std::string& ResourceContainer::GetName() const
{
	return m_name;
}

ActivityPriority_t ResourceContainer::GetPriority() const
{
	if (!m_manager.lock()->IsEnabled()) {
		return m_manager.lock()->GetDisabledPriority();
	}

	if (m_entities.empty()) {
		return m_manager.lock()->GetDefaultPriority();
	} else {
		ActivityPriority_t priority = (*std::max_element(m_entities.begin(),
			m_entities.end(), EntityPriorityCompare))->GetPriority();
		if (priority == ActivityPriorityNone) {
			return m_manager.lock()->GetDefaultPriority();
		} else {
			return priority;
		}
	}
}

bool ResourceContainer::IsFocused() const
{
	if (m_entities.empty()) {
		return false;
	} else {
		return (std::find_if(m_entities.begin(), m_entities.end(),
			boost::mem_fn(&BusEntity::IsFocused)) != m_entities.end());
	}
}

MojErr ResourceContainer::ToJson(MojObject& rep) const
{
	MojErr err = rep.putString(_T("name"), m_name.c_str());
	MojErrCheck(err);

	err = rep.putString(_T("priority"), ActivityPriorityNames[GetPriority()]);
	MojErrCheck(err);

	err = rep.putBool(_T("focused"), IsFocused());
	MojErrCheck(err);

	MojObject entities(MojObject::TypeArray);

	std::for_each(m_entities.begin(), m_entities.end(),
		boost::bind(&BusEntity::PushJson, _1, boost::ref(entities), false));

	err = rep.put(_T("entities"), entities);
	MojErrCheck(err);

	return MojErrNone;
}

void ResourceContainer::PushJson(MojObject& array) const
{
	MojErr errs = MojErrNone;

	MojObject rep(MojObject::TypeObject);

	MojErr err = ToJson(rep);
	MojErrAccumulate(errs, err);

	err = array.push(rep);
	MojErrAccumulate(errs, err);

	if (errs) {
		throw std::runtime_error("Unable to convert resource container to "
			"JSON object");
	}
}

bool ResourceContainer::EntityPriorityCompare(
	const boost::shared_ptr<BusEntity>& x,
	const boost::shared_ptr<BusEntity>& y)
{
	bool fx = x->IsFocused();
	bool fy = y->IsFocused();

	if (fx == fy) {
		return (x->GetPriority() < y->GetPriority());
	} else {
		return fx < fy;
	}
}

