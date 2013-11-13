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

#include "BusEntity.h"
#include "Activity.h"
#include "Logging.h"

#include <stdexcept>

MojLogger BusEntity::s_log(_T("activitymanager.entity"));

BusEntity::BusEntity(const BusId& id)
	: m_id(id)
{
}

BusEntity::~BusEntity()
{
}

void BusEntity::AssociateActivity(boost::shared_ptr<Activity> activity)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] associating with [Entity %s]",
		activity->GetId(), m_id.GetString().c_str());

	ActivityAssociations::insert_commit_data id;
	std::pair<ActivityAssociations::iterator, bool> result =
		m_associations.insert_check(activity,
			BusEntity::ActivityAssociationComp(), id);
	if (!result.second) {
		LOG_DEBUG("[Activity %llu] already associated with [Entity %s]",
			activity->GetId(), m_id.GetString().c_str());
		/* Don't throw here.  Be resilient. */
	} else {
		boost::shared_ptr<ActivitySetAutoAssociation> association =
			boost::make_shared<BusEntityAssociation>(activity,
				shared_from_this());
		m_associations.insert(*association);
		activity->AddEntityAssociation(association);
	}
}

void BusEntity::DissociateActivity(boost::shared_ptr<Activity> activity)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] dissociating from [Entity %s]",
		activity->GetId(), m_id.GetString().c_str());

	ActivityAssociations::iterator found = m_associations.find(activity,
		BusEntity::ActivityAssociationComp());
	if (found == m_associations.end()) {
		LOG_DEBUG("[Activity %llu] is not currently associated with [Entity %s]",
			activity->GetId(), m_id.GetString().c_str());
		/* Don't throw here.  Be resilient. */
	} else {
		/* Remember, removing the association will kill it */
		boost::shared_ptr<ActivitySetAutoAssociation> association =
			boost::dynamic_pointer_cast<ActivitySetAutoAssociation,
				ActivityAutoAssociation>(found->shared_from_this());

		m_associations.erase(found);

		activity->RemoveEntityAssociation(association);
	}
}

std::string BusEntity::GetName() const
{
	return m_id.GetString();
}

ActivityPriority_t BusEntity::GetPriority() const
{
	if (m_associations.empty()) {
		return ActivityPriorityNone;
	} else {
		return m_associations.begin()->GetActivity()->GetPriority();
	}
}

bool BusEntity::IsFocused() const
{
	if (m_associations.empty()) {
		return false;
	} else {
		return m_associations.begin()->GetActivity()->IsFocused();
	}
}


MojErr BusEntity::ToJson(MojObject& rep, bool includeActivities) const
{
	MojErr err = m_id.ToJson(rep);
	MojErrCheck(err);

	if (includeActivities) {
		MojObject activities(MojObject::TypeArray);

		std::for_each(m_associations.begin(), m_associations.end(),
			boost::bind(&Activity::PushIdentityJson,
				boost::bind<boost::shared_ptr<const Activity> >
					(&ActivitySetAutoAssociation::GetActivity, _1),
				boost::ref(activities)));

		err = rep.put(_T("activities"), activities);
		MojErrCheck(err);
	}

	err = rep.putString(_T("priority"), ActivityPriorityNames[GetPriority()]);
	MojErrCheck(err);

	err = rep.putBool(_T("focused"), IsFocused());
	MojErrCheck(err);

	return MojErrNone;
}

void BusEntity::PushJson(MojObject& array, bool includeActivities) const
{
	MojErr errs = MojErrNone;

	MojObject entityJson(MojObject::TypeObject);

	MojErr err = ToJson(entityJson, includeActivities);
	MojErrAccumulate(errs, err);

	err = array.push(entityJson);
	MojErrAccumulate(errs, err);

	if (errs) {
		throw std::runtime_error("Unable to convert from BusEntity to JSON "
			"object");
	}
}

void BusEntity::ReassociateActivity(
	boost::shared_ptr<ActivitySetAutoAssociation> association)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Updating association between [Activity %llu] and [Entity %s]",
		association->GetActivity()->GetId(),
		m_id.GetString().c_str());

	if (association->m_link.is_linked()) {
		association->m_link.unlink();
	} else {
		LOG_DEBUG("Association between [Activity %llu] and [Entity %s] was not linked",
			association->GetActivity()->GetId(),
			m_id.GetString().c_str());
	}

	std::pair<ActivityAssociations::iterator, bool> result =
		m_associations.insert(*association);
	if (!result.second) {
		LOG_WARNING(MSGID_ASSOSCIATN_UPDATE_FAIL, 3, PMLOGKS("Reason","another association already present"),
			  PMLOGKFV("Activity","%llu",association->GetActivity()->GetId()),
			  PMLOGKS("Entity",m_id.GetString().c_str()), "");
	}
}

bool BusEntity::ActivityPriorityCompare(
	const boost::shared_ptr<const Activity>& x,
	const boost::shared_ptr<const Activity>& y)
{
	/* Establish total order.  Sort based on focus first, then priority
	 * highest at the front.  Determining what the priority of the associated
	 * entity is as easy as looking at the first element in the set */

	bool fx, fy;

	fx = x->IsFocused();
	fy = y->IsFocused();

	if (fx == fy) {
		ActivityPriority_t px, py;

		px = x->GetPriority();
		py = y->GetPriority();

		if (px == py) {
			/* Just a total order at this point, priorities are equal */
			return x < y;
		} else {
			return px > py;
		}
	} else {
		return fx > fy;
	}
}

bool BusEntity::ActivityAssociationComp::operator()(
	const ActivitySetAutoAssociation& association,
	const boost::shared_ptr<const Activity>& activity) const
{
	return BusEntity::ActivityPriorityCompare(association.GetActivity(),
		activity);
}

bool BusEntity::ActivityAssociationComp::operator()(
	const boost::shared_ptr<const Activity>& activity,
	const ActivitySetAutoAssociation& association) const
{
	return BusEntity::ActivityPriorityCompare(activity,
		association.GetActivity());
}

BusEntity::BusEntityAssociation::BusEntityAssociation(
	boost::shared_ptr<Activity> activity, boost::shared_ptr<BusEntity> entity)
	: ActivitySetAutoAssociation(activity)
	, m_entity(entity)
{
}

BusEntity::BusEntityAssociation::~BusEntityAssociation()
{
}

bool BusEntity::BusEntityAssociation::operator<(
	const ActivitySetAutoAssociation& rhs) const
{
	return BusEntity::ActivityPriorityCompare(GetActivity(), rhs.GetActivity());
}

void BusEntity::BusEntityAssociation::Reassociate()
{
	m_entity.lock()->ReassociateActivity(
		boost::dynamic_pointer_cast<ActivitySetAutoAssociation,
			ActivityAutoAssociation>(shared_from_this()));
}

std::string BusEntity::BusEntityAssociation::GetTargetName() const
{
	return m_entity.lock()->GetName();
}

