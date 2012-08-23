// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "PowerdScheduler.h"

#include <stdexcept>

const char *PowerdScheduler::PowerdWakeupKey =
	"com.palm.activitymanager.wakeup";

PowerdScheduler::PowerdScheduler(MojService *service)
	: m_service(service)
{
}

PowerdScheduler::~PowerdScheduler()
{
}

void PowerdScheduler::ScheduledWakeup()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Powerd wakeup callback received"));

	Wake();
}

void PowerdScheduler::Enable()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Powerd scheduler enabled"));

	MonitorSystemTime();
}

void PowerdScheduler::UpdateTimeout(time_t nextWakeup, time_t curTime)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Updating powerd scheduling callback - "
		"nextWakeup %llu, current time %llu"),
		(unsigned long long)nextWakeup, (unsigned long long)curTime);

	MojErr err;
	MojErr errs = MojErrNone;

	MojObject params;

	params.putBool("wakeup", true);

	char formattedTime[32];
	FormatWakeupTime(nextWakeup, formattedTime, sizeof(formattedTime));

	err = params.putString(_T("at"), formattedTime);
	MojErrAccumulate(errs, err);

	err = params.putString(_T("key"), PowerdWakeupKey);
	MojErrAccumulate(errs, err);

	err = params.putString(_T("uri"),
		"palm://com.palm.activitymanager/callback/scheduledwakeup");
	MojErrAccumulate(errs, err);

	MojObject cbParams(MojObject::TypeObject);
	err = params.put(_T("params"), cbParams);
	MojErrAccumulate(errs, err);

	if (errs) {
		MojLogError(s_log, _T("Error constructing parameters for "
			"powerd set timeout call"));
		throw std::runtime_error("Error constructing parameters for "
			"powerd set timeout call");
	}

	m_call = boost::make_shared<MojoWeakPtrCall<PowerdScheduler> >(
		boost::dynamic_pointer_cast<PowerdScheduler, Scheduler>(
			shared_from_this()), &PowerdScheduler::HandleTimeoutSetResponse,
		m_service, "palm://com.palm.power/timeout/set", params);
	m_call->Call();
}

void PowerdScheduler::CancelTimeout()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Cancelling powerd timeout"));

	MojObject params;

	MojErr err = params.putString(_T("key"), PowerdWakeupKey);

	if (err) {
		MojLogError(s_log, _T("Error constructing parameters for powerd "
			"clear timeout call"));
		throw std::runtime_error("Error constructing parameters for powerd "
			"clear timeout call");
	}

	m_call = boost::make_shared<MojoWeakPtrCall<PowerdScheduler> >(
		boost::dynamic_pointer_cast<PowerdScheduler, Scheduler>(
			shared_from_this()), &PowerdScheduler::HandleTimeoutClearResponse,
		m_service, "palm://com.palm.power/timeout/clear", params);
	m_call->Call();
}

void PowerdScheduler::MonitorSystemTime()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Subscribing to System Time change notifications"));

	MojObject params;
	MojErr err = params.putBool(_T("subscribe"), true);
	if (err) {
		MojLogError(s_log, _T("Error constructing parameters for subscription "
			"to System Time Service"));
		throw std::runtime_error("Error constructing parameters for "
			"subscription to System Time Service");
	}

	m_systemTimeCall = boost::make_shared<MojoWeakPtrCall<PowerdScheduler> >(
		boost::dynamic_pointer_cast<PowerdScheduler, Scheduler>(
			shared_from_this()), &PowerdScheduler::HandleSystemTimeResponse,
		m_service, "palm://com.palm.systemservice/time/getSystemTime", params,
		MojoCall::Unlimited);
	m_systemTimeCall->Call();
}

size_t PowerdScheduler::FormatWakeupTime(time_t wake, char *at,
	size_t len) const
{
	/* "MM/DD/YY HH:MM:SS" (UTC) */

	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	gmtime_r(&wake, &tm);
	return strftime(at, len, "%m/%d/%Y %H:%M:%S", &tm);
}

void PowerdScheduler::HandleTimeoutSetResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Timeout Set response %s"),
		MojoObjectJson(response).c_str());

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("Failed to register scheduled wakeup: %s"),
				MojoObjectJson(response).c_str());
		} else {
			MojLogWarning(s_log, _T("Failed to register scheduled wakeup, "
				"retrying: %s"), MojoObjectJson(response).c_str());
			m_call->Call();
			return;
		}
	} else {
		MojLogInfo(s_log, _T("Successfully registered scheduled wakeup"));
	}

	m_call.reset();
}

void PowerdScheduler::HandleTimeoutClearResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Timeout Clear response %s"),
		MojoObjectJson(response).c_str());

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("Failed to cancel scheduled wakeup: %s"),
				MojoObjectJson(response).c_str());
		} else {
			MojLogWarning(s_log, _T("Failed to cancel scheduled wakeup, "
				"retrying: %s"), MojoObjectJson(response).c_str());
			m_call->Call();
			return;
		}
	} else {
		MojLogInfo(s_log, _T("Successfully cancelled scheduled wakeup"));
	}

	m_call.reset();
}

void PowerdScheduler::HandleSystemTimeResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("System Time response %s"),
		MojoObjectJson(response).c_str());

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("System Time subscription experienced "
				"an uncorrectable failure: %s"),
				MojoObjectJson(response).c_str());
			m_systemTimeCall.reset();
		} else {
			MojLogWarning(s_log, _T("System Time subscription failed, "
				"retrying: %s"), MojoObjectJson(response).c_str());
			MonitorSystemTime();
		}
		return;
	}

	MojInt64 localOffset;

	bool found = response.get(_T("offset"), localOffset);
	if (!found) {
		MojLogWarning(s_log, _T("System Time message is missing timezone "
			"offset"));
	} else {
		MojLogInfo(s_log, _T("System Time timezone offset: %lld"),
			(long long)localOffset);

		localOffset *= 60;

		SetLocalOffset((off_t)localOffset);
	}

	/* The time changed response can also trip if the actual time(2) has
	 * been updated, which should cause a recalculation also of the
	 * interval schedules (at least if 'skip' is set) */
	TimeChanged();
}

