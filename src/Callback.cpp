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

#include "Callback.h"
#include "Activity.h"

MojLogger Callback::s_log(_T("activitymanager.callback"));

Callback::Callback(boost::shared_ptr<Activity> activity)
	: m_serial(UnassignedSerial)
	, m_activity(activity)
{
}

Callback::~Callback()
{
}

void Callback::Failed(FailureType failure)
{
	if (!m_activity.expired()) {
		m_activity.lock()->CallbackFailed(shared_from_this(), failure);
	}
}

void Callback::Succeeded()
{
	if (!m_activity.expired()) {
		m_activity.lock()->CallbackSucceeded(shared_from_this());
	}
}

unsigned Callback::GetSerial() const
{
	return m_serial;
}

void Callback::SetSerial(unsigned serial)
{
	m_serial = serial;
}

