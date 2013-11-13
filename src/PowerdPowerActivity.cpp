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

#include "PowerdPowerActivity.h"
#include "Activity.h"
#include "MojoCall.h"
#include "Logging.h"
#include <sstream>

#ifdef ACTIVITYMANAGER_RENEW_POWER_ACTIVITIES
const int PowerdPowerActivity::PowerActivityLockDuration = 300;
const int PowerdPowerActivity::PowerActivityLockUpdateInterval = 240;
#else
const int PowerdPowerActivity::PowerActivityLockDuration = 900;
const int PowerdPowerActivity::PowerActivityLockUpdateInterval = 900;
#endif

const int PowerdPowerActivity::PowerActivityDebounceDuration = 12;

PowerdPowerActivity::PowerdPowerActivity(
	boost::shared_ptr<PowerManager> manager,
	boost::shared_ptr<Activity> activity, MojService *service,
	unsigned long serial)
	: PowerActivity(manager, activity)
	, m_service(service)
	, m_serial(serial)
	, m_currentState(PowerUnlocked)
	, m_targetState(PowerUnlocked)
{
}

PowerdPowerActivity::~PowerdPowerActivity()
{
	if (m_currentState != PowerUnlocked) {
		LOG_DEBUG("Power Activity serial %lu destroyed while power is not unlocked", m_serial);
	}
}

PowerActivity::PowerState PowerdPowerActivity::GetPowerState() const
{
	return m_currentState;
}

void PowerdPowerActivity::Begin()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] Locking power on",
		m_activity.lock()->GetId());

	if ((m_currentState == PowerLocked) || (m_targetState == PowerLocked)) {
		return;
	}

	if (m_timeout) {
		m_timeout.reset();
	}

	/* If there's a command in flight, cancel it. */
	if (m_call) {
		m_call.reset();
	}

	m_targetState = PowerLocked;
	m_currentState = PowerUnknown;

	MojErr err = CreateRemotePowerActivity();
	if (err) {
		LOG_ERROR(MSGID_PWR_LOCK_CREATE_FAIL,1,
			PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
			"Failed to issue command to create power lock");

		/* Fake it so the Activity doesn't stall */
		LOG_WARNING(MSGID_PWR_LOCK_FAKE_NOTI,1,
			PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
			"Faking power locked notification");

		m_currentState = PowerLocked;
		m_activity.lock()->PowerLockedNotification();
	}
}

void PowerdPowerActivity::End()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] Unlocking power",
		m_activity.lock()->GetId());

	if ((m_currentState == PowerUnlocked) || (m_targetState == PowerUnlocked)) {
		return;
	}

	if (m_timeout) {
		m_timeout.reset();
	}

	/* If there's a command in flight, cancel it. */
	if (m_call) {
		m_call.reset();
	}

	m_targetState = PowerUnlocked;
	m_currentState = PowerUnknown;

	bool debounce = m_activity.lock()->IsPowerDebounce();

	MojErr err;
	if (debounce) {
		err = CreateRemotePowerActivity(true);
	} else {
		err = RemoveRemotePowerActivity();
	}

	if (err) {
		LOG_ERROR(MSGID_PWR_UNLOCK_CREATE_FAIL,1,
			PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
			"Failed to issue command to %s power unlock", debounce ? "debounce" : "remove");

		/* Fake it so the Activity doesn't stall - the lock will fall off
		 * on its own... not that that's a good thing, but better than
		 * nothing. */
		LOG_WARNING(MSGID_PWR_UNLOCK_FAKE_NOTI,1,
			PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
			"Faking power unlocked notification");

		m_currentState = PowerUnlocked;
		m_activity.lock()->PowerUnlockedNotification();
	}
}

unsigned long PowerdPowerActivity::GetSerial() const
{
	return m_serial;
}

void PowerdPowerActivity::PowerLockedNotification(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	if (err == MojErrNone) {
		if (m_currentState != PowerLocked) {
			LOG_DEBUG("[Activity %llu] Power lock successfully created",
				m_activity.lock()->GetId());

			m_currentState = PowerLocked;
			m_activity.lock()->PowerLockedNotification();
		} else {
			LOG_DEBUG("[Activity %llu] Power lock successfully updated",
				m_activity.lock()->GetId());
		}

		if (!m_timeout) {
			m_timeout = boost::make_shared<Timeout<PowerdPowerActivity> >(
				boost::dynamic_pointer_cast<PowerdPowerActivity, PowerActivity>(
					shared_from_this()), PowerActivityLockUpdateInterval,
					&PowerdPowerActivity::TimeoutNotification);
		}

		m_timeout->Arm();

		m_call.reset();
	} else {
		if (m_currentState != PowerLocked) {
			LOG_WARNING(MSGID_PWRLK_NOTI_CREATE_FAIL,1,
				PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
				"Attempt to create power lock failed, retrying. Error %s",
				 MojoObjectJson(response).c_str());
		} else {
			LOG_WARNING(MSGID_PWRLK_NOTI_UPDATE_FAIL,1,
				PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
				"Attempt to update power lock failed, retrying. Error %s",
				MojoObjectJson(response).c_str());
		}

		m_call.reset();

		/* Retry - powerd may have restarted. */
		MojErr err2 = CreateRemotePowerActivity();
		if (err2) {
			LOG_WARNING(MSGID_PWR_LOCK_CREATE_FAIL,1,
				PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
				"Failed to issue command to create power lock in Noti" );

			/* If power was not currently locked, fake the create so the
			 * Activity doesn't hang */
			if (m_currentState != PowerLocked) {
				LOG_WARNING(MSGID_PWR_LOCK_FAKE_NOTI,1,
					PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
					"Faking power locked notification in noti");

				m_currentState = PowerLocked;
				m_activity.lock()->PowerLockedNotification();
			}
		}
	}
}

void PowerdPowerActivity::PowerUnlockedNotification(MojServiceMessage *msg,
	const MojObject& response, MojErr err, bool debounce)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	if (err == MojErrNone) {
		LOG_DEBUG("[Activity %llu] Power lock successfully %s",
			m_activity.lock()->GetId(), debounce ? "debounced" : "removed");

		// reset the call *before* the unlocked notification; if
		// we end up requeuing the activity due to a transient, we may end
		// up sending a new call.
		m_call.reset();

		m_currentState = PowerUnlocked;
		m_activity.lock()->PowerUnlockedNotification();
	} else {
		LOG_WARNING(MSGID_PWRULK_NOTI_ERR,1,
			PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
			"Attempt to %s power lock failed, retrying. Error: %s",
			debounce ? "debounce" : "remove",
			MojoObjectJson(response).c_str());

		m_call.reset();

		/* Retry. XXX but if error is "Activity doesn't exist", it's done. */
		MojErr err2;
		if (debounce) {
			err2 = CreateRemotePowerActivity(true);
		} else {
			err2 = RemoveRemotePowerActivity();
		}

		if (err2) {
			LOG_WARNING(MSGID_PWR_UNLOCK_CREATE_ERR,1,
				PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
				"Failed to issue command to %s power lock",
				debounce ? "debounce" : "remove");

			/* Not much to do at this point, let the Activity move on
			 * so it doesn't hang. */
			LOG_WARNING(MSGID_PWR_UNLOCK_FAKE_NOTI,1,
				PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
				"Faking power unlocked notification in Noti");

			m_currentState = PowerUnlocked;
			m_activity.lock()->PowerUnlockedNotification();
		}
	}
}

void PowerdPowerActivity::PowerUnlockedNotificationNormal(
	MojServiceMessage *msg, const MojObject& response, MojErr err)
{
	PowerUnlockedNotification(msg, response, err, false);
}

void PowerdPowerActivity::PowerUnlockedNotificationDebounce(
	MojServiceMessage *msg, const MojObject& response, MojErr err)
{
	PowerUnlockedNotification(msg, response, err, true);
}


void PowerdPowerActivity::TimeoutNotification()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] Attempting to update power lock",
		m_activity.lock()->GetId());

#ifdef ACTIVITYMANAGER_RENEW_POWER_ACTIVITIES
	MojErr err = CreateRemotePowerActivity();
	if (err) {
		LOG_ERROR(MSGID_PWR_ACTIVITY_CREATE_ERR,1,
			PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
			"Failed to issue command to update power lock");
		m_timeout->Arm();
	}
#else
	LOG_WARNING(MSGID_PWR_TIMEOUT_NOTI,2,
	    PMLOGKFV("Activity","%llu",m_activity.lock()->GetId()),
	    PMLOGKFV("LOCKDURATION","%d",PowerActivityLockDuration),
	    "Power Activity exceeded maximum length of seconds, not being renewed");
#endif
}

std::string PowerdPowerActivity::GetRemotePowerActivityName() const
{
	boost::shared_ptr<Activity> activity = m_activity.lock();
	std::stringstream name;

	/* am-<creator>-<activityId>.<serial> */
	name << "am:" << activity->GetCreator().GetId() << "-" <<
		activity->GetId() << "." << m_serial;

	return name.str();
}

MojErr PowerdPowerActivity::CreateRemotePowerActivity(bool debounce)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err;
	MojObject params;

	err = params.putString(_T("id"), GetRemotePowerActivityName().c_str());
	MojErrCheck(err);

	int duration;
	if (debounce) {
		duration = PowerActivityDebounceDuration * 1000;
	} else {
		duration = PowerActivityLockDuration * 1000;
	}

	err = params.putInt(_T("duration_ms"), duration);
	MojErrCheck(err);

	m_call = boost::make_shared<MojoWeakPtrCall<PowerdPowerActivity> >(
		boost::dynamic_pointer_cast<PowerdPowerActivity, PowerActivity>(
			shared_from_this()), debounce ?
		&PowerdPowerActivity::PowerUnlockedNotificationDebounce :
		&PowerdPowerActivity::PowerLockedNotification,
		m_service, "palm://com.palm.power/com/palm/power/activityStart",
		params);
	m_call->Call();

	return MojErrNone;
}

MojErr PowerdPowerActivity::RemoveRemotePowerActivity()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err;
	MojObject params;

	err = params.putString(_T("id"), GetRemotePowerActivityName().c_str());
	MojErrCheck(err);

	m_call = boost::make_shared<MojoWeakPtrCall<PowerdPowerActivity> >(
		boost::dynamic_pointer_cast<PowerdPowerActivity, PowerActivity>(
			shared_from_this()),
		&PowerdPowerActivity::PowerUnlockedNotificationNormal, m_service,
		"palm://com.palm.power/com/palm/power/activityEnd", params);
	m_call->Call();

	return MojErrNone;
}

