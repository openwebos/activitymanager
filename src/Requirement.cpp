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

#include "Requirement.h"
#include "Activity.h"
#include "ActivityJson.h"

Requirement::Requirement(boost::shared_ptr<Activity> activity, bool met)
	: m_activity(activity)
	, m_met(met)
{
}

Requirement::~Requirement()
{
}

void Requirement::Met()
{
	if (m_met)
		return;

	m_met = true;

	m_activity.lock()->RequirementMet(shared_from_this());
}

void Requirement::Unmet()
{
	if (!m_met)
		return;

	m_met = false;

	m_activity.lock()->RequirementUnmet(shared_from_this());
}

void Requirement::Updated()
{
	m_activity.lock()->RequirementUpdated(shared_from_this());
}

bool Requirement::IsMet() const
{
	return m_met;
}

boost::shared_ptr<Activity> Requirement::GetActivity()
{
	return m_activity.lock();
}

boost::shared_ptr<const Activity> Requirement::GetActivity() const
{
	return m_activity.lock();
}

ListedRequirement::ListedRequirement(boost::shared_ptr<Activity> activity,
	bool met)
	: Requirement(activity, met)
{
}

ListedRequirement::~ListedRequirement()
{
}

RequirementCore::RequirementCore(const std::string& name,
	const MojObject& value, bool met)
	: m_name(name)
	, m_value(value)
	, m_met(met)
{
}

RequirementCore::~RequirementCore()
{
}

const std::string& RequirementCore::GetName() const
{
	return m_name;
}

MojErr RequirementCore::ToJson(MojObject& rep, unsigned flags) const
{
	MojErr err = MojErrNone;

	if (flags & ACTIVITY_JSON_CURRENT) {
		if (m_current.type() != MojObject::TypeUndefined) {
			err = rep.put(m_name.c_str(), m_current);
			MojErrCheck(err);
		} else {
			err = rep.putBool(m_name.c_str(), IsMet());
			MojErrCheck(err);
		}
	} else {
		err = rep.put(m_name.c_str(), m_value);
		MojErrCheck(err);
	}

	return err;
}

bool RequirementCore::SetCurrentValue(const MojObject& current)
{
	if (m_current != current) {
		m_current = current;
		return true;
	} else {
		return false;
	}
}

void RequirementCore::Met()
{
	m_met = true;
}

void RequirementCore::Unmet()
{
	m_met = false;
}

bool RequirementCore::IsMet() const
{
	return m_met;
}

BasicCoreListedRequirement::BasicCoreListedRequirement(
	boost::shared_ptr<Activity> activity,
	boost::shared_ptr<const RequirementCore> core, bool met)
	: ListedRequirement(activity, met)
	, m_core(core)
{
}

BasicCoreListedRequirement::~BasicCoreListedRequirement()
{
}

const std::string& BasicCoreListedRequirement::GetName() const
{
	return m_core->GetName();
}

MojErr BasicCoreListedRequirement::ToJson(MojObject& rep, unsigned flags) const
{
	return m_core->ToJson(rep, flags);
}

