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

#include "LunaBusProxy.h"
#include "ContainerManager.h"

#include "MojoCall.h"

#include <stdexcept>

MojLogger LunaBusProxy::s_log(_T("activitymanager.lunabus"));

LunaBusProxy::LunaBusProxy(
	boost::shared_ptr<ContainerManager> containerManager, MojService *service)
	: m_service(service)
	, m_containerManager(containerManager)
{
	InitializeRestrictedIds();
}

LunaBusProxy::~LunaBusProxy()
{
}

void LunaBusProxy::Enable()
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Enabling Luna Bus Proxy"));

	EnableBusUpdates();
}

void LunaBusProxy::Disable()
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Disabling Luna Bus Proxy"));

	m_busUpdates->Cancel();
	m_busUpdates.reset();
}

void LunaBusProxy::EnableBusUpdates()
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Enabling service updates from Luna bus"));

	MojObject params;
	MojErr err = params.putString(_T("category"), "_private_service_status");
	if (err) {
		throw std::runtime_error("Failed to initialize parameters to enable "
			"Luna bus updates");
	}

	m_busUpdates = boost::make_shared<MojoWeakPtrCall<LunaBusProxy> >(
		shared_from_this(), &LunaBusProxy::ProcessBusUpdate,
		m_service, "palm://com.palm.bus/signal/addmatch", params,
		MojoCall::Unlimited);
	m_busUpdates->Call();
}

void LunaBusProxy::ProcessBusUpdate(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Received Luna Bus Update: %s"),
		MojoObjectJson(response).c_str());

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("Subscription to Luna Bus updates "
				"experienced an uncorrectable failure: %s"),
				MojoObjectJson(response).c_str());
			m_busUpdates.reset();
		} else {
			MojLogWarning(s_log, _T("Subscription to Luna Bus updates failed, "
				"resubscribing: %s"), MojoObjectJson(response).c_str());
			EnableBusUpdates();
		}
		return;
	}

	if (response.contains(_T("services"))) {
		MojObject services;

		response.get(_T("services"), services);
		if (services.type() != MojObject::TypeArray) {
			MojLogWarning(s_log, _T("Badly formatted Luna bus update... "
				"services is not an array: %s"),
				MojoObjectJson(response).c_str());
			return;
		}

		for (MojObject::ConstArrayIterator iter = services.arrayBegin();
			iter != services.arrayEnd(); ++iter) {
			const MojObject& service = *iter;

			MojErr err2;
			bool found = false;
			MojString serviceName;
			err2 = service.get(_T("serviceName"), serviceName, found);
			if (err2 || !found) {
				MojLogWarning(s_log, _T("Failed to retreive service name "
					"while processing batch service status update"));
				continue;
			}

			MojObject busIds;
			found = service.get(_T("allNames"), busIds);
			if (!found) {
				MojLogWarning(s_log, _T("Failed to retreive equivalent "
					"service names for service %s while processing batch "
					"service status update"), serviceName.data());
				continue;
			}

			MojUInt32 pid;
			found = false;
			err2 = service.get(_T("pid"), pid, found);
			if (err2 || !found) {
				MojLogWarning(s_log, _T("Failed to retreive process id for "
					"service \"%s\" while processing batch service status "
					"update"), serviceName.data());
				continue;
			}

			ContainerManager::BusIdVec busIdVec;
			busIdVec.reserve(busIds.size());
			PopulateBusIds(busIds, busIdVec, serviceName);

			if (!busIdVec.empty()) {
				if (ContainsRestricted(busIdVec)) {
					MojLogDebug(s_log, _T("Restricted names found in list of "
						"equivalent names for service \"%s\""),
						serviceName.data());
				} else {
					m_containerManager->MapContainer(busIdVec[0].GetId(),
						busIdVec, (pid_t)pid);
				}
			} else {
				MojLogWarning(s_log, _T("No valid names found for service "
					"\"%s\" while processing batch service status update"),
					serviceName.data());
			}
		}
	}

	if (response.contains(_T("connected"))) {
		bool isConnected = false;

		bool found = response.get(_T("connected"), isConnected);
		if (!found) {
			MojLogWarning(s_log, _T("Failed to retreive connected state "
				"processing service update from Luna bus hub"));
			return;
		}

		MojString serviceName;
		found = false;
		MojErr err2 = response.get(_T("serviceName"), serviceName, found);
		if (err2 || !found) {
			MojLogWarning(s_log, _T("Failed to retreive service name while "
				"processing service update from Luna bus hub"));
			return;
		}

		if (!isConnected) {
			MojLogDebug(s_log, _T("Disconnect message for service name \"%s\""),
				serviceName.data());
			return;
		}

		MojObject busIds;
		found = response.get(_T("allNames"), busIds);
		if (!found) {
			MojLogWarning(s_log, _T("Failed to retreive equivalent "
				"service names for service \"%s\" while processing service "
				"connect update"), serviceName.data());
			return;
		}

		MojUInt32 pid;
		found = false;
		err2 = response.get(_T("pid"), pid, found);
		if (err2 || !found) {
			MojLogWarning(s_log, _T("Failed to retreive pid for service \"%s\" "
				"while processing connect update from Luna bus"),
				serviceName.data());
			return;
		}

		ContainerManager::BusIdVec busIdVec;
		busIdVec.reserve(busIds.size());
		PopulateBusIds(busIds, busIdVec, serviceName);

		if (!busIdVec.empty()) {
			if (ContainsRestricted(busIdVec)) {
				MojLogDebug(s_log, _T("Restricted names found in list of "
					"equivalent names for service \"%s\""), serviceName.data());
			} else {
				m_containerManager->MapContainer(busIdVec[0].GetId(), busIdVec,
					(pid_t)pid);
			}
		} else {
			MojLogDebug(s_log, _T("No valid names found for service "
				"\"%s\" while processing service connect update"),
				serviceName.data());
		}
	}
}

void LunaBusProxy::PopulateBusIds(const MojObject& ids,
	ContainerManager::BusIdVec& busIdVec, const MojString& serviceName) const
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Transforming JSON array %s into BusIds"),
		MojoObjectJson(ids).c_str());

	if (ids.type() != MojObject::TypeArray) {
		MojLogWarning(s_log, _T("Equivalent bus names not passed as an array "
			"of ids: %s"), MojoObjectJson(ids).c_str());
		throw std::runtime_error("Bus names not passed as an array of ids");
	}

	bool foundServiceName = false;

	for (MojObject::ConstArrayIterator iter = ids.arrayBegin();
		iter != ids.arrayEnd(); ++iter) {
		const MojObject& id = *iter;

		MojString busIdStr;
		MojErr err = id.stringValue(busIdStr);
		if (err) {
			MojLogWarning(s_log, _T("Failed to retreive bus id from JSON "
				"object %s"), MojoObjectJson(id).c_str());
			continue;
		}

		if (serviceName == busIdStr) {
			foundServiceName = true;
		}

		if (!FilterName(busIdStr)) {
			busIdVec.push_back(BusId(busIdStr.data(), BusService));
		}
	}

	if (!foundServiceName) {
		if (!FilterName(serviceName)) {
			busIdVec.push_back(BusId(serviceName.data(), BusService));
		}
	}
}

bool LunaBusProxy::FilterName(const MojString& name) const
{
	if (name.empty()) {
		return true;
	} else if (!strncmp(name.data(), "/var", 4)) {
		/* If name is an anonymous name, skip */
		return true;
	} else if (strchr(name.data(), '*')) {
		/* Or if it contains wild cards... */
		return true;
	}

	return false;
}

void LunaBusProxy::InitializeRestrictedIds()
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Initializing list of restricted bus ids that "
		"should not be managed"));

	static const char *ids[] = {
		"com.palm.db", "com.palm.systemmanager", "com.palm.filecache",
		"com.palm.activitymanager", "com.palm.hidd", "com.palm.telephony"
	};

	static const int numIds = sizeof(ids)/sizeof(const char *);

	for (int i = 0; i < numIds; ++i) {
		m_restrictedIds.insert(BusId(ids[i], BusService));
	}

	static const char *patterns[] = {
		"com\\.palm\\.mediad.*"
	};

	static const int numPatterns = sizeof(patterns)/sizeof(const char *);

	for (int i = 0; i < numPatterns; ++i) {
		m_restrictedPatterns.push_back(boost::regex(patterns[i],
			boost::regex::icase | boost::regex::optimize));
	}
}

bool LunaBusProxy::ContainsRestricted(
	const ContainerManager::BusIdVec& busIdVec) const
{
	for (ContainerManager::BusIdVec::const_iterator iter = busIdVec.begin();
		iter != busIdVec.end(); ++iter)
	{
		if (m_restrictedIds.find(*iter) != m_restrictedIds.end()) {
			return true;
		}

		for (RestrictedPatternList::const_iterator patternIter =
			m_restrictedPatterns.begin();
			patternIter != m_restrictedPatterns.end(); ++patternIter) {
			if (boost::regex_match(iter->GetId(), *patternIter)) {
				return true;
			}
		}
	}

	return false;
}

