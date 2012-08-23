/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef __ACTIVITYMANAGER_ACTIVITYAUTOASSOCIATION_H__
#define __ACTIVITYMANAGER_ACTIVITYAUTOASSOCIATION_H__

#include "Base.h"

#include <boost/intrusive/set.hpp>
#include <string>

class Activity;

/*
 * Combination of a weak pointer and an intrusive data structure.
 *
 * This class provides a clean way to reference an Activity in sorted
 * data structures with a weak reference to the Activity where the
 * association in the data structure will automatically be removed in the
 * event that the Activity is released.
 */
class ActivityAutoAssociation : public
	boost::enable_shared_from_this<ActivityAutoAssociation>
{
public:
	ActivityAutoAssociation(boost::shared_ptr<Activity> activity);
	virtual ~ActivityAutoAssociation();

	boost::shared_ptr<Activity> GetActivity();
	boost::shared_ptr<const Activity> GetActivity() const;

	virtual std::string GetTargetName() const = 0;

protected:
	boost::weak_ptr<Activity>	m_activity;
};

class ActivitySetAutoAssociation : public ActivityAutoAssociation
{
public:
	ActivitySetAutoAssociation(boost::shared_ptr<Activity> activity);
	virtual ~ActivitySetAutoAssociation();

	/* Establish a total order for the set */
	virtual bool operator<(const ActivitySetAutoAssociation& rhs) const = 0;

	/* The Associated Activities position in the set changed. */
	virtual void Reassociate() = 0;

	typedef boost::intrusive::set_member_hook<
		boost::intrusive::link_mode<
			boost::intrusive::auto_unlink> >	SetLink;

	SetLink	m_link;

	typedef boost::intrusive::member_hook<ActivitySetAutoAssociation,
		ActivitySetAutoAssociation::SetLink,
		&ActivitySetAutoAssociation::m_link>	SetLinkOption;
	typedef boost::intrusive::set<ActivitySetAutoAssociation, SetLinkOption,
		boost::intrusive::constant_time_size<false> >	AutoAssociationSet;
};

#endif /* __ACTIVITYMANAGER_ACTIVITYAUTOASSOCIATION_H__ */
