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

#ifndef __ACTIVITYMANAGER_REQUIREMENT_H__
#define __ACTIVITYMANAGER_REQUIREMENT_H__

#include "Base.h"

#include <boost/intrusive/list.hpp>

#include <core/MojString.h>
#include <core/MojErr.h>

class Activity;

class Requirement : public boost::enable_shared_from_this<Requirement>
{
public:
	Requirement(boost::shared_ptr<Activity> activity, bool met = false);
	virtual ~Requirement();

	void Met();
	void Unmet();
	void Updated();

	bool IsMet() const;

	boost::shared_ptr<Activity> GetActivity();
	boost::shared_ptr<const Activity> GetActivity() const;

	virtual const std::string& GetName() const = 0;

	virtual MojErr ToJson(MojObject& rep, unsigned flags) const = 0;

protected:
	typedef boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<
			boost::intrusive::auto_unlink> >	RequirementListItem;
	RequirementListItem	m_activityListItem;

	friend class Activity;

	boost::weak_ptr<Activity>	m_activity;

	bool	m_met;
};

class ListedRequirement : public Requirement
{
public:
	ListedRequirement(boost::shared_ptr<Activity> activity, bool met = false);
	virtual ~ListedRequirement();

protected:
	RequirementListItem	m_managerListItem;

	friend class RequirementManager;
};

class RequirementCore {
public:
	RequirementCore(const std::string& name, const MojObject& value,
		bool met = false);
	~RequirementCore();

	virtual const std::string& GetName() const;
	virtual MojErr ToJson(MojObject& rep, unsigned flags) const;

	virtual bool SetCurrentValue(const MojObject& current);

	void Met();
	void Unmet();

	bool IsMet() const;

protected:
	std::string	m_name;
	MojObject	m_value;
	MojObject	m_current;

	bool		m_met;
};

class BasicCoreListedRequirement : public ListedRequirement
{
public:
	BasicCoreListedRequirement(boost::shared_ptr<Activity> activity,
		boost::shared_ptr<const RequirementCore> core, bool met = false);
	virtual ~BasicCoreListedRequirement();

	virtual const std::string& GetName() const;

	virtual MojErr ToJson(MojObject& rep, unsigned flags) const;

protected:
	boost::shared_ptr<const RequirementCore>	m_core;
};

#endif /* __ACTIVITYMANAGER_REQUIREMENT_H__ */
