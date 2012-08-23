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

#include "SystemManagerProxy.h"
#include "ActivityManager.h"
#include "Activity.h"
#include "MojoCall.h"

#include <stdexcept>
#include <time.h>

MojLogger SystemManagerProxy::s_log(_T("activitymanager.systemmanagerproxy"));

SystemManagerProxy::SystemManagerProxy(MojService *service,
	boost::shared_ptr<ActivityManager> am)
	: m_service(service)
	, m_am(am)
	, m_bootIssued(false)
{
	m_bootupRequirementCore =
		boost::make_shared<RequirementCore>("bootup", true);
}

SystemManagerProxy::~SystemManagerProxy()
{
}

const std::string& SystemManagerProxy::GetName() const
{
	static std::string name("SystemManagerProxy");
	return name;
}

boost::shared_ptr<Requirement> SystemManagerProxy::InstantiateRequirement(
	boost::shared_ptr<Activity> activity, const std::string& name,
	const MojObject& value)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Instantiating [Requirement %s] for [Activity %llu]"),
		name.c_str(), activity->GetId());

	if (name == "bootup") {
		if ((value.type() == MojObject::TypeBool) && value.boolValue()) {
			boost::shared_ptr<ListedRequirement> req =
				boost::make_shared<BasicCoreListedRequirement>(
					activity, m_bootupRequirementCore);
			m_bootupRequirements.push_back(*req);
			return req;
		} else {
			throw std::runtime_error("If 'bootup' requirement is specified, "
				"the only legal value is 'true'");
		}
	} else {
		MojLogError(s_log, _T("[Manager %s] does not know how to instantiate "
			"[Requirement %s] for [Activity %llu]"), GetName().c_str(),
			name.c_str(), activity->GetId());
		throw std::runtime_error("Attempt to instantiate unknown requirement");
	}
}

void SystemManagerProxy::RegisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	MojLogTrace(s_log);
	MojLogNotice(s_log, _T("Registering requirements"));

	master->RegisterRequirement("bootup", shared_from_this());
}

void SystemManagerProxy::UnregisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	MojLogTrace(s_log);
	MojLogNotice(s_log, _T("Unregistering requirements"));

	master->UnregisterRequirement("bootup", shared_from_this());
}

void SystemManagerProxy::Enable()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Enabling System Manager Proxy"));

	MojObject params;
	params.putBool(_T("subscribe"), true);

	m_bootstatus = boost::make_shared<MojoWeakPtrCall<SystemManagerProxy> >(
		boost::dynamic_pointer_cast<SystemManagerProxy, RequirementManager>
			(shared_from_this()),
		&SystemManagerProxy::BootStatusUpdate, m_service,
		"palm://com.palm.systemmanager/getBootStatus", params,
		MojoCall::Unlimited);
	m_bootstatus->Call();
}

void SystemManagerProxy::Disable()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Disabling System Manager Proxy"));

	m_bootstatus.reset();
}

/*
 * palm://com.palm.systemmanager/getBootStatus
 *
 * {
 *     "finished" : <bool>
 *     "firstUse" : <bool>
 * }
 */
void SystemManagerProxy::BootStatusUpdate(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Boot status update message: %s"),
		MojoObjectJson(response).c_str());

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("Subscription to System Manager "
				"experienced an uncorrectable failure: %s"),
				MojoObjectJson(response).c_str());
			m_bootstatus.reset();
			/* XXX Kick start if it hasn't been, for resilience?  Or
			 * fail-secure? (Might want to fail that way for OTA
			 * data migration) */
		} else {
			MojLogWarning(s_log, _T("Subscription to System Manager failed, "
				"retrying: %s"), MojoObjectJson(response).c_str());
			static struct timespec sleep = { 0, 250000000};
			nanosleep(&sleep, NULL);
			m_bootstatus->Call();
		}
		return;
	}

	bool finished = false;
	bool found;

	found = response.get(_T("finished"), finished);
	if (!found) {
		MojLogWarning(s_log, _T("Bootup status not returned by System "
			"Manager: %s"), MojoObjectJson(response).c_str());
	} else {
		if (finished) {
			if (!m_bootIssued) {
				/* Trip the requirement, once.  Then no one else will get it
				 * until the system goes down and comes back up.  (This will
				 * include a LunaSysMgr restart). */
				std::for_each(m_bootupRequirements.begin(),
					m_bootupRequirements.end(),
					boost::mem_fn(&Requirement::Met));
				m_bootIssued = true;
			}

			m_am->Enable(ActivityManager::UI_ENABLE);
		} else {
			/* If finished goes back to false, reset the flag and be willing
			 * to trigger the bootup events again */
			if (m_bootIssued) {
				m_bootIssued = false;
			}

			m_am->Disable(ActivityManager::UI_ENABLE);
		}
	}
}

