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

#ifndef __ACTIVITYMANAGER_BUSENTITY_H__
#define __ACTIVITYMANAGER_BUSENTITY_H__

#include "Base.h"
#include "BusId.h"
#include "ActivityTypes.h"
#include "ActivityAutoAssociation.h"

#include <set>

class Activity;

/* Represents an App or Service on the Bus that has running Activities
 * associated with it.  Tracks all of its running Activities, sorted
 * by priority, and its current priority.
 *
 * A single BusEntity may ultimate be mapped into multiple Resource
 * Containers of various different types (mem, cpu, io).  The Container
 * Manager for each type of resource should track that mapping, and notify
 * containers if their entities' associations change (as the entities don't
 * know what containers they're mapped into and therefore can't generate those
 * notifications themselves.
 */
class BusEntity : public boost::enable_shared_from_this<BusEntity> {
public:
	BusEntity(const BusId& id);
	virtual ~BusEntity();

	void AssociateActivity(boost::shared_ptr<Activity> activity);
	void DissociateActivity(boost::shared_ptr<Activity> activity);

	std::string GetName() const;
	ActivityPriority_t GetPriority() const;
	bool IsFocused() const;

	MojErr ToJson(MojObject& rep, bool includeActivities) const;
	void PushJson(MojObject& array, bool includeActivities) const;

protected:
	void ReassociateActivity(boost::shared_ptr<ActivitySetAutoAssociation>
		association);

	typedef ActivitySetAutoAssociation::AutoAssociationSet
		ActivityAssociations;

	static bool ActivityPriorityCompare(
		const boost::shared_ptr<const Activity>& x,
		const boost::shared_ptr<const Activity>& y);

	struct ActivityAssociationComp {
		bool operator()(const ActivitySetAutoAssociation& association,
			const boost::shared_ptr<const Activity>& activity) const;
		bool operator()(const boost::shared_ptr<const Activity>& activity,
			const ActivitySetAutoAssociation& association) const;
	};

	class BusEntityAssociation : public ActivitySetAutoAssociation
	{
	public:
		BusEntityAssociation(boost::shared_ptr<Activity> activity,
			boost::shared_ptr<BusEntity> entity);
		virtual ~BusEntityAssociation();

		virtual bool operator<(const ActivitySetAutoAssociation& rhs) const;
		virtual void Reassociate();

		virtual std::string GetTargetName() const;

	protected:
		boost::weak_ptr<BusEntity>	m_entity;
	};

	/* Track all Activities whos priority should effect the priority of
	 * this entity's (App or Service) container. */
	ActivityAssociations	m_associations;

	BusId	m_id;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_MANAGEDBUSENTITY_H__ */
