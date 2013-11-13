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

#include "DefaultRequirementManager.h"
#include "Requirement.h"
#include "Activity.h"
#include "Logging.h"

#include <stdexcept>

MojLogger DefaultRequirementManager::s_log(_T("activitymanager.defaultrequirementmanager"));

DefaultRequirementManager::DefaultRequirementManager()
{
	m_neverRequirementCore =
		boost::make_shared<RequirementCore>("never", true);
}

DefaultRequirementManager::~DefaultRequirementManager()
{
}

const std::string& DefaultRequirementManager::GetName() const
{
	static std::string name("DefaultRequirementManager");
	return name;
}

boost::shared_ptr<Requirement>
DefaultRequirementManager::InstantiateRequirement(
	boost::shared_ptr<Activity> activity, const std::string& name,
	const MojObject& value)
{
	if (name == "never") {
		if ((value.type() == MojObject::TypeBool) && value.boolValue()) {
			boost::shared_ptr<ListedRequirement> req =
				boost::make_shared<BasicCoreListedRequirement>(
					activity, m_neverRequirementCore);
			m_neverRequirements.push_back(*req);
			return req;
		} else {
			throw std::runtime_error("If a \"never\" requirement is "
				"specified, the only legal value is 'true'");
		}
	} else {
		LOG_ERROR(MSGID_REQUIREMENT_INSTANTIATE_FAIL, 3, PMLOGKS("Manager",GetName().c_str()),
			  PMLOGKFV("Activity","%llu",activity->GetId()), PMLOGKS("Requirement",name.c_str()), "");
		throw std::runtime_error("Attempt to instantiate unknown requirement");
	}
}

void DefaultRequirementManager::RegisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Registering requirements");

	master->RegisterRequirement("never", shared_from_this());
}

void DefaultRequirementManager::UnregisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Unregistering requirements");

	master->UnregisterRequirement("never", shared_from_this());
}

