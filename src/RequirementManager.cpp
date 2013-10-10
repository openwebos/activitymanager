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

#include "RequirementManager.h"
#include "DefaultRequirementManager.h"

#include <algorithm>
#include <stdexcept>

MojLogger MasterRequirementManager::s_log(_T("activitymanager.masterrequirementmanager"));

RequirementManager::RequirementManager()
{
}

RequirementManager::~RequirementManager()
{
}

void RequirementManager::RegisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
}

void RequirementManager::UnregisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
}

void RequirementManager::Enable()
{
}

void RequirementManager::Disable()
{
}

MasterRequirementManager::MasterRequirementManager()
{
}

MasterRequirementManager::~MasterRequirementManager()
{
}

const std::string& MasterRequirementManager::GetName() const
{
	static std::string name("MasterRequirementManager");
	return name;
}

boost::shared_ptr<Requirement> MasterRequirementManager::InstantiateRequirement(
	boost::shared_ptr<Activity> activity, const std::string& name,
	const MojObject& value)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Looking up specific requirement manager to "
		"instantiate [Requirement %s]"), name.c_str());

	RequirementMap::iterator found = m_requirements.find(name);
	if (found == m_requirements.end()) {
		MojLogWarning(s_log, _T("Manager for [Requirement %s] not found"),
			name.c_str());
		throw std::runtime_error("Manager for requirement not found");
	}

	return found->second->InstantiateRequirement(activity, name, value);
}

void MasterRequirementManager::RegisterRequirement(const std::string& name,
	boost::shared_ptr<RequirementManager> manager)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Registering [Manager %s] for [Requirement %s]"),
		manager->GetName().c_str(), name.c_str());

	m_requirements[name] = manager;
}

void MasterRequirementManager::UnregisterRequirement(const std::string& name,
	boost::shared_ptr<RequirementManager> manager)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Unregistering [Manager %s] from [Requirement %s]"),
		manager->GetName().c_str(), name.c_str());

	RequirementMap::iterator found = m_requirements.find(name);
	if (found == m_requirements.end()) {
		MojLogWarning(s_log, _T("No manager found for [Requirement %s] "
			"attempting to unregister [Manager %s]"), name.c_str(),
			manager->GetName().c_str());
	} else if (found->second != manager) {
		MojLogWarning(s_log, _T("[Manager %s] not currently registered for "
			"[Requirement %s], it is registered to [Manager %s]"),
			manager->GetName().c_str(), name.c_str(),
			found->second->GetName().c_str());
	} else {
		m_requirements.erase(found);
	}
}

void MasterRequirementManager::Enable()
{
	std::for_each(m_managers.begin(), m_managers.end(),
		boost::mem_fn(&RequirementManager::Enable));
}

void MasterRequirementManager::Disable()
{
	std::for_each(m_managers.begin(), m_managers.end(),
		boost::mem_fn(&RequirementManager::Disable));
}

void MasterRequirementManager::AddManager(
	boost::shared_ptr<RequirementManager> manager)
{
	ManagerSet::iterator found = m_managers.find(manager);
	if (found != m_managers.end()) {
		throw std::runtime_error("Requirement Manager is already a "
			"member of the manager set");
	}

	m_managers.insert(manager);

	manager->RegisterRequirements(
		boost::dynamic_pointer_cast<MasterRequirementManager,
			RequirementManager>(shared_from_this()));
}

void MasterRequirementManager::RemoveManager(
	boost::shared_ptr<RequirementManager> manager)
{
	ManagerSet::iterator found = m_managers.find(manager);
	if (found != m_managers.end()) {
		throw std::runtime_error("Requirement Manager is not in the "
			"manager set");
	}

	m_managers.erase(found);

	manager->UnregisterRequirements(
		boost::dynamic_pointer_cast<MasterRequirementManager,
			RequirementManager>(shared_from_this()));
}

