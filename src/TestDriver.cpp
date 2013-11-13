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

#include "TestDriver.h"
#include "MojoCall.h"
#include "Logging.h"
#include <stdexcept>

MojLogger TestDriver::s_log(_T("activitymanager.testdriver"));

TestDriver::TestDriver(MojService *service)
	: m_service(service)
{
}

TestDriver::~TestDriver()
{
}

void TestDriver::Start(MojObject& response)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject activity;

	bool found = response.get(_T("$activity"), activity);
	if (!found) {
		throw std::runtime_error("Failed to find $activity property in "
			"response");
	}

	found = activity.get(_T("activityId"), m_activityId);
	if (!found) {
		throw std::runtime_error("Failed to locate activityId property in "
			"response");
	}

	MojObject params;
	MojErr err = params.putInt(_T("activityId"), m_activityId);
	if (err) {
		throw std::runtime_error("Failed to initialize call parameters for "
			"adopt call");
	}

	err = params.putBool(_T("subscribe"), true);
	if (err) {
		throw std::runtime_error("Failed to initialize subscribe parameter "
			"for adopt call");
	}

	m_adopt = boost::make_shared<MojoWeakPtrCall<TestDriver> >(
		shared_from_this(), &TestDriver::HandleAdoptResponse, m_service,
		"palm://com.palm.activitymanager/adopt", params,
		MojoCall::Unlimited);
	m_adopt->Call();
}

void TestDriver::HandleAdoptResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Adopt Response: %s",
		MojoObjectJson(response).c_str());

	if (err) {
		LOG_DEBUG("Adopt failed");
		m_adopt.reset();
		return;
	}

	MojObject params;

	MojErr err2 = params.put(_T("activityId"), m_activityId);
	if (err2) {
		LOG_DEBUG("Failed to initialize args for complete call");
		m_adopt.reset();
		return;
	}

	m_complete = boost::make_shared<MojoWeakPtrCall<TestDriver> >(
		shared_from_this(), &TestDriver::HandleCompleteResponse, m_service,
		"palm://com.palm.activitymanager/complete", params);
	m_complete->Call();
}

void TestDriver::HandleCompleteResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Complete Response: %s",
		MojoObjectJson(response).c_str());

	m_complete.reset();
	m_adopt.reset();
}

