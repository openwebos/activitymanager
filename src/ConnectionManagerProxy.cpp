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

#include "ConnectionManagerProxy.h"
#include "Activity.h"
#include "MojoCall.h"

#include <core/MojServiceRequest.h>

#include <stdexcept>

MojLogger ConnectionManagerProxy::s_log(_T("activitymanager.connectionproxy"));

ConnectionManagerProxy::ConnectionManagerProxy(MojService *service)
	: m_service(service)
	, m_internetConfidence(ConnectionConfidenceUnknown)
	, m_wifiConfidence(ConnectionConfidenceUnknown)
	, m_wanConfidence(ConnectionConfidenceUnknown)
{
	m_internetRequirementCore = boost::make_shared<RequirementCore>
		("internet", true);
	m_wifiRequirementCore = boost::make_shared<RequirementCore>
		("wifi", true);
	m_wanRequirementCore = boost::make_shared<RequirementCore>
		("wan", true);

	MojErr err = ConnectionConfidenceUnknownName.assign("unknown");
	if (err != MojErrNone) {
		throw std::runtime_error("Error initializing \"unknown\" connection "
			"confidence name string");
	}

	const char *nameStrings[ConnectionConfidenceMax] =
		{ "none", "poor", "fair", "excellent" };

	for (int i = 0; i < ConnectionConfidenceMax; ++i) {
		MojErr err = ConnectionConfidenceNames[i].assign(nameStrings[i]);
		if (err != MojErrNone) {
			throw std::runtime_error("Error initializing connection confidence "
				"name strings");
		}

		m_internetConfidenceCores[i] = boost::make_shared<RequirementCore>
			("internetConfidence", ConnectionConfidenceNames[i]);
		m_wanConfidenceCores[i] = boost::make_shared<RequirementCore>
			("wanConfidence", ConnectionConfidenceNames[i]);
		m_wifiConfidenceCores[i] = boost::make_shared<RequirementCore>
			("wifiConfidence", ConnectionConfidenceNames[i]);
	}
}

ConnectionManagerProxy::~ConnectionManagerProxy()
{
}

const std::string& ConnectionManagerProxy::GetName() const
{
	static std::string name("ConnectionManagerProxy");
	return name;
}

boost::shared_ptr<Requirement> ConnectionManagerProxy::InstantiateRequirement(
	boost::shared_ptr<Activity> activity, const std::string& name,
	const MojObject& value)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Instantiating [Requirement %s] for [Activity %llu]"),
		name.c_str(), activity->GetId());

	if (name == "internet") {
		if ((value.type() == MojObject::TypeBool) && value.boolValue()) {
			boost::shared_ptr<ListedRequirement> req =
				boost::make_shared<BasicCoreListedRequirement>(
					activity, m_internetRequirementCore,
					m_internetRequirementCore->IsMet());
			m_internetRequirements.push_back(*req);
			return req;
		} else {
			throw std::runtime_error("If an 'internet' requirement is "
				"specified, the only legal value is 'true'");
		}
	} else if (name == "internetConfidence") {
		return InstantiateConfidenceRequirement(activity,
			m_internetConfidenceCores, m_internetConfidenceRequirements,
			value);
	} else if (name == "wan") {
		if ((value.type() == MojObject::TypeBool) && value.boolValue()) {
			boost::shared_ptr<ListedRequirement> req =
				boost::make_shared<BasicCoreListedRequirement>(
					activity, m_wanRequirementCore,
					m_wanRequirementCore->IsMet());
			m_wanRequirements.push_back(*req);
			return req;
		} else {
			throw std::runtime_error("If an 'wan' requirement is "
				"specified, the only legal value is 'true'");
		}
	} else if (name == "wanConfidence") {
		return InstantiateConfidenceRequirement(activity, m_wanConfidenceCores,
			m_wanConfidenceRequirements, value);
	} else if (name == "wifi") {
		if ((value.type() == MojObject::TypeBool) && value.boolValue()) {
			boost::shared_ptr<ListedRequirement> req =
				boost::make_shared<BasicCoreListedRequirement>(
					activity, m_wifiRequirementCore,
					m_wifiRequirementCore->IsMet());
			m_wifiRequirements.push_back(*req);
			return req;
		} else {
			throw std::runtime_error("If an 'wifi' requirement is "
				"specified, the only legal value is 'true'");
		}
	} else if (name == "wifiConfidence") {
		return InstantiateConfidenceRequirement(activity, m_wifiConfidenceCores,
			m_wifiConfidenceRequirements, value);
	} else {
		MojLogError(s_log, _T("[Manager %s] does not know how to instantiate "
			"[Requirement %s] for [Activity %llu]"), GetName().c_str(),
			name.c_str(), activity->GetId());
		throw std::runtime_error("Attempt to instantiate unknown requirement");
	}
}

void ConnectionManagerProxy::RegisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Registering requirements"));

	master->RegisterRequirement("internet", shared_from_this());
	master->RegisterRequirement("wifi", shared_from_this());
	master->RegisterRequirement("wan", shared_from_this());
	master->RegisterRequirement("internetConfidence", shared_from_this());
	master->RegisterRequirement("wifiConfidence", shared_from_this());
	master->RegisterRequirement("wanConfidence", shared_from_this());
}

void ConnectionManagerProxy::UnregisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Unregistering requirements"));

	master->UnregisterRequirement("internet", shared_from_this());
	master->UnregisterRequirement("wifi", shared_from_this());
	master->UnregisterRequirement("wan", shared_from_this());
	master->UnregisterRequirement("internetConfidence", shared_from_this());
	master->UnregisterRequirement("wifiConfidence", shared_from_this());
	master->UnregisterRequirement("wanConfidence", shared_from_this());
}

void ConnectionManagerProxy::Enable()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Enabling Connection Manager Proxy"));

	MojObject params;
	params.putBool(_T("subscribe"), true);

	m_call = boost::make_shared<MojoWeakPtrCall<ConnectionManagerProxy> >(
		boost::dynamic_pointer_cast<ConnectionManagerProxy, RequirementManager>
			(shared_from_this()),
		&ConnectionManagerProxy::ConnectionManagerUpdate, m_service,
		"palm://com.palm.connectionmanager/getStatus", params,
		MojoCall::Unlimited);
	m_call->Call();
}

void ConnectionManagerProxy::Disable()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Disabling Connection Manager Proxy"));

	m_call.reset();
}

/*
 * palm://com.palm.connectionmanager/getStatus
 *
 * {
 *    "isInternetConnectionAvailable" : <bool>
 *    "wifi" : {
 *        "state" : "connected"|"disconnected"
 *        "ipAddress" : <string>
 *        "ssid" : <string>
 *        "bssid" : <string>
 *    }
 *
 *    "wan" : {
 *        "state" : "connected"|"disconnected"
 *        "ipAddress" : <string>
 *        "network" : "unknown" | "unusable" | "gprs" | "edge" | "umts" ...
 *			(unusable means WAN is available but blocked due to a call)
 *	  }
 *
 * }
 */
void ConnectionManagerProxy::ConnectionManagerUpdate(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("Subscription to Connection Manager "
				"experienced an uncorrectable failure: %s"),
				MojoObjectJson(response).c_str());
			m_call.reset();
		} else {
			MojLogWarning(s_log, _T("Subscription to Connection Manager "
				"failed, resubscribing: %s"), MojoObjectJson(response).c_str());
			m_call->Call();
		}
		return;
	}

	MojLogInfo(s_log, _T("Update from Connection Manager: %s"),
		MojoObjectJson(response).c_str());

	bool isInternetConnectionAvailable = false;
	response.get(_T("isInternetConnectionAvailable"),
		isInternetConnectionAvailable);

	bool updated = m_internetRequirementCore->SetCurrentValue(response);

	if (isInternetConnectionAvailable) {
		if (!m_internetRequirementCore->IsMet()) {
			MojLogInfo(s_log, _T("Internet connection is now available"));
			m_internetRequirementCore->Met();
			std::for_each(m_internetRequirements.begin(),
				m_internetRequirements.end(),
				boost::mem_fn(&Requirement::Met));
		} else if (updated) {
			std::for_each(m_internetRequirements.begin(),
				m_internetRequirements.end(),
				boost::mem_fn(&Requirement::Updated));
		}
	} else {
		if (m_internetRequirementCore->IsMet()) {
			MojLogInfo(s_log, _T("Internet connection is no longer "
				"available"));
			m_internetRequirementCore->Unmet();
			std::for_each(m_internetRequirements.begin(),
				m_internetRequirements.end(),
				boost::mem_fn(&Requirement::Unmet));
		}
	}

	UpdateWifiStatus(response);
	UpdateWANStatus(response);

	int maxConfidence = (m_wifiConfidence > m_wanConfidence) ?
		m_wifiConfidence : m_wanConfidence;
	if (m_internetConfidence != maxConfidence) {
		m_internetConfidence = maxConfidence;
		MojLogInfo(s_log, _T("Internet confidence level changed to %d"),
			m_internetConfidence);
		UpdateConfidenceRequirements(m_internetConfidenceCores,
			m_internetConfidenceRequirements, m_internetConfidence);
	}
}

MojErr ConnectionManagerProxy::UpdateWifiStatus(const MojObject& response)
{
	MojErr err;
	bool wifiAvailable = false;
	bool updated = false;
	MojInt64 confidence = (MojInt64)ConnectionConfidenceUnknown;

	MojObject wifi;
	bool found = response.get(_T("wifi"), wifi);
	if (found) {
		updated = m_wifiRequirementCore->SetCurrentValue(wifi);

		MojString wifiConnected;
		found = false;
		err = wifi.get(_T("state"), wifiConnected, found);
		MojErrCheck(err);

		if (!found) {
			MojLogWarning(s_log, _T("Wifi connection status not returned by "
				"Connection Manager"));
		} else if (wifiConnected == "connected") {
			MojString onInternet;
			err = wifi.get(_T("onInternet"), onInternet, found);
			MojErrCheck(err);

			if (onInternet == "yes") {
				wifiAvailable = true;
				confidence = GetConfidence(wifi);
			}
		}
	} else {
		MojLogWarning(s_log, _T("Wifi status not returned by Connection "
			"Manager"));
	}

	if (wifiAvailable) {
		if (!m_wifiRequirementCore->IsMet()) {
			MojLogInfo(s_log, _T("Wifi connection is now available"));
			m_wifiRequirementCore->Met();
			std::for_each(m_wifiRequirements.begin(), m_wifiRequirements.end(),
				boost::mem_fn(&Requirement::Met));
		} else if (updated) {
			std::for_each(m_wifiRequirements.begin(), m_wifiRequirements.end(),
				boost::mem_fn(&Requirement::Updated));
		}
	} else {
		if (m_wifiRequirementCore->IsMet()) {
			MojLogInfo(s_log, _T("Wifi connection is no longer available"));
			m_wifiRequirementCore->Unmet();
			std::for_each(m_wifiRequirements.begin(), m_wifiRequirements.end(),
				boost::mem_fn(&Requirement::Unmet));
		}
	}

	if (m_wifiConfidence != (int)confidence) {
		m_wifiConfidence = (int)confidence;
		MojLogInfo(s_log, _T("Wifi confidence level changed to %d"),
			m_wifiConfidence);
		UpdateConfidenceRequirements(m_wifiConfidenceCores,
			m_wifiConfidenceRequirements, m_wifiConfidence);
	}

	return MojErrNone;
}

MojErr ConnectionManagerProxy::UpdateWANStatus(const MojObject& response)
{
	MojErr err;
	bool wanAvailable = false;
	bool updated = false;
	MojInt64 confidence = (MojInt64)ConnectionConfidenceUnknown;

	MojObject wan;
	bool found = response.get(_T("wan"), wan);
	if (found) {
		updated = m_wanRequirementCore->SetCurrentValue(wan);

		MojString wanConnected;
		found = false;
		err = wan.get(_T("state"), wanConnected, found);
		MojErrCheck(err);

		if (!found) {
			MojLogWarning(s_log, _T("WAN connection status not returned by "
				"Connection Manager"));
		} else if (wanConnected == "connected") {
			MojString wanNetwork;
			found = false;
			err = wan.get(_T("network"), wanNetwork, found);
			MojErrCheck(err);

			if (!found) {
				MojLogWarning(s_log, _T("WAN network mode not returned by "
					"Connection Manager"));
			} else if (wanNetwork != "unusable") {
				MojString onInternet;
				err = wan.get(_T("onInternet"), onInternet, found);
				MojErrCheck(err);

				if (onInternet == "yes") {
					wanAvailable = true;
					confidence = GetConfidence(wan);
				}
			}
		}
	} else {
		MojLogWarning(s_log, _T("WAN status not returned by Connection "
			"Manager"));
	}

	if (wanAvailable) {
		if (!m_wanRequirementCore->IsMet()) {
			MojLogInfo(s_log, _T("WAN connection is now available"));
			m_wanRequirementCore->Met();
			std::for_each(m_wanRequirements.begin(), m_wanRequirements.end(),
				boost::mem_fn(&Requirement::Met));
		} else if (updated) {
			std::for_each(m_wanRequirements.begin(), m_wanRequirements.end(),
				boost::mem_fn(&Requirement::Updated));
		}
	} else {
		if (m_wanRequirementCore->IsMet()) {
			MojLogInfo(s_log, _T("WAN connection is no longer available"));
			m_wanRequirementCore->Unmet();
			std::for_each(m_wanRequirements.begin(), m_wanRequirements.end(),
				boost::mem_fn(&Requirement::Unmet));
		}
	}

	if (m_wanConfidence != (int)confidence) {
		m_wanConfidence = (int)confidence;
		MojLogInfo(s_log, _T("WAN confidence level changed to %d"),
			m_wanConfidence);
		UpdateConfidenceRequirements(m_wanConfidenceCores,
			m_wanConfidenceRequirements, m_wanConfidence);
	}

	return MojErrNone;
}

int ConnectionManagerProxy::GetConfidence(const MojObject& spec) const
{
	MojObject confidenceObj;

	bool found = spec.get(_T("networkConfidenceLevel"),
		confidenceObj);
	if (!found) {
		MojLogWarning(s_log, _T("Failed to retreive network confidence from "
			"network description %s"), MojoObjectJson(spec).c_str());
	} else {
		return ConfidenceToInt(confidenceObj);

	}

	return ConnectionConfidenceUnknown;
}

int ConnectionManagerProxy::ConfidenceToInt(const MojObject& confidenceObj) const
{
	if (confidenceObj.type() != MojObject::TypeString) {
		MojLogWarning(s_log, _T("Network confidence must be specified as a "
			"string"));
		return ConnectionConfidenceUnknown;
	}

	MojString confidenceStr;
	MojErr err = confidenceObj.stringValue(confidenceStr);
	if (err != MojErrNone) {
		MojLogWarning(s_log, _T("Failed to retreive network confidence level "
			"as string"));
		return ConnectionConfidenceUnknown;
	}

	for (int i = 0; i < ConnectionConfidenceMax; ++i) {
		if (confidenceStr == ConnectionConfidenceNames[i]) {
			return i;
		}
	}

	MojLogWarning(s_log, _T("Unknown connection confidence string: "
		"\"%s\""), confidenceStr.data());

	return ConnectionConfidenceUnknown;
}

boost::shared_ptr<Requirement>
ConnectionManagerProxy::InstantiateConfidenceRequirement(
	boost::shared_ptr<Activity> activity,
	boost::shared_ptr<RequirementCore> *confidenceCores,
	RequirementList* confidenceLists, const MojObject& confidenceDesc)
{
	int confidence = ConfidenceToInt(confidenceDesc);

	if (confidence == ConnectionConfidenceUnknown) {
		throw std::runtime_error("Invalid connection confidence level "
			"specified");
	}

	if ((confidence < 0) || (confidence >= ConnectionConfidenceMax)) {
		throw std::runtime_error("Confidence out of range");
	}

	boost::shared_ptr<ListedRequirement> req =
		boost::make_shared<BasicCoreListedRequirement>(
			activity, confidenceCores[confidence],
			confidenceCores[confidence]->IsMet());
	confidenceLists[confidence].push_back(*req);
	return req;
}

void ConnectionManagerProxy::UpdateConfidenceRequirements(
	boost::shared_ptr<RequirementCore> *confidenceCores,
	RequirementList *confidenceLists, int confidence)
{
	if (((confidence < 0) || (confidence >= ConnectionConfidenceMax)) &&
		(confidence != ConnectionConfidenceUnknown)) {
		MojLogWarning(s_log, _T("Unknown connection confidence level %d seen "
			"attempting to update confidence requirements"), confidence);
		return;
	}

	MojString& confidenceName = (confidence == ConnectionConfidenceUnknown) ?
		ConnectionConfidenceUnknownName :
		ConnectionConfidenceNames[confidence];

	for (int i = 0; i < ConnectionConfidenceMax; ++i) {
		confidenceCores[i]->SetCurrentValue(MojObject(confidenceName));

		if (confidence < i) {
			if (confidenceCores[i]->IsMet()) {
				confidenceCores[i]->Unmet();
				std::for_each(confidenceLists[i].begin(),
					confidenceLists[i].end(),
					boost::mem_fn(&Requirement::Unmet));
			} else {
				std::for_each(confidenceLists[i].begin(),
					confidenceLists[i].end(),
					boost::mem_fn(&Requirement::Updated));
			}
		} else {
			if (!confidenceCores[i]->IsMet()) {
				confidenceCores[i]->Met();
				std::for_each(confidenceLists[i].begin(),
					confidenceLists[i].end(),
					boost::mem_fn(&Requirement::Met));
			} else {
				std::for_each(confidenceLists[i].begin(),
					confidenceLists[i].end(),
					boost::mem_fn(&Requirement::Updated));
			}
		}
	}
}

