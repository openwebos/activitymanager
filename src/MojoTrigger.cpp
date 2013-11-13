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

#include "MojoTrigger.h"
#include "MojoTriggerSubscription.h"
#include "MojoMatcher.h"
#include "Activity.h"
#include "ActivityJson.h"
#include "Logging.h"

#include <stdexcept>

MojLogger MojoTrigger::s_log(_T("activitymanager.trigger"));

MojoTrigger::MojoTrigger(boost::shared_ptr<MojoMatcher> matcher)
	: m_matcher(matcher)
{
}

MojoTrigger::~MojoTrigger()
{
}

void MojoTrigger::SetSubscription(
	boost::shared_ptr<MojoTriggerSubscription> subscription)
{
	m_subscription = subscription;
}

MojErr MojoTrigger::ToJson(boost::shared_ptr<const Activity> activity,
	MojObject& rep, unsigned flags) const
{
	MojErr err;

	err = m_matcher->ToJson(rep, flags);
	MojErrCheck(err);

	if (m_subscription) {
		err = m_subscription->ToJson(rep, flags);
		MojErrCheck(err);
	}

	return MojErrNone;
}


MojoExclusiveTrigger::MojoExclusiveTrigger(
	boost::shared_ptr<Activity> activity,
	boost::shared_ptr<MojoMatcher> matcher)
	: MojoTrigger(matcher)
	, m_triggered(false)
	, m_activity(activity)
{
}

MojoExclusiveTrigger::~MojoExclusiveTrigger()
{
}

void MojoExclusiveTrigger::Arm(boost::shared_ptr<Activity> activity)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	if (activity != m_activity.lock()) {
		LOG_ERROR(MSGID_CANT_TRIGGER_DIFF_ACTIVITY,2, PMLOGKFV("activity","%llu", activity->GetId()),
			PMLOGKFV("Owner_Activity","%llu", m_activity.lock()->GetId()), "");
		throw std::runtime_error("Can't arm trigger for a different "
			"Activity");
	}

	if (!m_subscription) {
		LOG_DEBUG("[Activity %llu] Can't arm trigger that has no subscription",
			m_activity.lock()->GetId()); // LATHA_TBD
		throw std::runtime_error("Can't arm trigger that has no subscription");
	}

	m_triggered = false;

	LOG_DEBUG("[Activity %llu] Arming Trigger on \"%s\"",
		m_activity.lock()->GetId(), m_subscription->GetURL().GetURL().data());

	if (!m_subscription->IsSubscribed()) {
		m_subscription->Subscribe();
	}
}

void MojoExclusiveTrigger::Disarm(boost::shared_ptr<Activity> activity)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	if (activity != m_activity.lock()) {
		LOG_WARNING(MSGID_DISARM_TRIGGER_FOR_ACTIVITY,2, PMLOGKFV("Activity","%llu",activity->GetId()),
			   PMLOGKFV("Owner_Activity","%llu",m_activity.lock()->GetId()), "");
		throw std::runtime_error("Can't disarm trigger for a different "
			"Activity");
	}

	if (m_subscription) {
		LOG_DEBUG("[Activity %llu] Disarming Trigger on \"%s\"",
			m_activity.lock()->GetId(),
			m_subscription->GetURL().GetURL().data());
	} else {
		LOG_DEBUG("[Activity %llu] Disarming Trigger",
			m_activity.lock()->GetId());
	}

	m_triggered = false;

	if (m_subscription && m_subscription->IsSubscribed()) {
		m_subscription->Unsubscribe();
	}
}

void MojoExclusiveTrigger::Fire()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] Trigger firing",
		m_activity.lock()->GetId());

	m_triggered = true;
	m_subscription->Unsubscribe();
	m_activity.lock()->Triggered(shared_from_this());
}

bool MojoExclusiveTrigger::IsArmed(
	boost::shared_ptr<const Activity> activity) const
{
	if (m_activity.lock() != activity) {
		LOG_WARNING(MSGID_DISARM_TRIGGER_FOR_ACTIVITY,2, PMLOGKFV("Activity","%llu",activity->GetId()),
			   PMLOGKFV("Owner_Activity","%llu",m_activity.lock()->GetId()), "");

		return false;
	}

	return m_subscription->IsSubscribed();
}

bool MojoExclusiveTrigger::IsTriggered(
	boost::shared_ptr<const Activity> activity) const
{
	if (m_activity.lock() != activity) {
		LOG_WARNING(MSGID_CHECK_TRIGGER_FIRED, 1,
		    PMLOGKFV("Owner_Activity","%llu",activity->GetId()), "");
		return false;
	}

	return m_triggered;
}

boost::shared_ptr<Activity> MojoExclusiveTrigger::GetActivity()
{
	return m_activity.lock();
}

boost::shared_ptr<const Activity> MojoExclusiveTrigger::GetActivity() const
{
	return m_activity.lock();
}

void MojoExclusiveTrigger::ProcessResponse(const MojObject& response,
	MojErr err)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	/* Subscription guarantees any errors received are from the subscribing
	 * Service.  Transient bus errors are handled automatically.
	 *
	 * XXX have an option to disable auto-resubscribe */
	if (err) {
		LOG_DEBUG("[Activity %llu] Trigger call \"%s\" failed",
			m_activity.lock()->GetId(),
			m_subscription->GetURL().GetURL().data());
		m_response = response;
		Fire();
	} else if (m_matcher->Match(response)) {
		LOG_DEBUG("[Activity %llu] Trigger call \"%s\" fired!",
			m_activity.lock()->GetId(),
			m_subscription->GetURL().GetURL().data());
		m_response = response;
		Fire();
	}
}

MojErr MojoExclusiveTrigger::ToJson(boost::shared_ptr<const Activity> activity,
	MojObject& rep, unsigned flags) const
{
	if (flags & ACTIVITY_JSON_CURRENT) {
		if (IsTriggered(activity)) {
			rep = m_response;
		} else {
			rep = MojObject(false);
		}

		return MojErrNone;
	} else {
		return MojoTrigger::ToJson(activity, rep, flags);
	}
}

