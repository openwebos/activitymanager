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

#include "PowerdProxy.h"
#include "PowerdPowerActivity.h"
#include "MojoCall.h"
#include "Activity.h"
#include "ActivityJson.h"

#include <stdexcept>

PowerdProxy::PowerdProxy(MojService *service)
	: m_chargerStatusSubscribed(false)
	, m_batteryStatusSubscribed(false)
	, m_inductiveChargerConnected(false)
	, m_usbChargerConnected(false)
	, m_onPuck(false)
	, m_batteryPercent(0)
	, m_service(service)
{
	m_chargingRequirementCore = boost::make_shared<RequirementCore>
		("charging", true);
	m_dockedRequirementCore = boost::make_shared<RequirementCore>
		("docked", true);
}

PowerdProxy::~PowerdProxy()
{
}

const std::string& PowerdProxy::GetName() const
{
	static std::string name("PowerdProxy");
	return name;
}

boost::shared_ptr<Requirement> PowerdProxy::InstantiateRequirement(
	boost::shared_ptr<Activity> activity, const std::string& name,
	const MojObject& value)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Instantiating [Requirement %s] for [Activity %llu]"),
		name.c_str(), activity->GetId());

	if (name == "charging") {
		if ((value.type() != MojObject::TypeBool) || !value.boolValue()) {
			throw std::runtime_error("A \"charging\" requirement must specify "
				"'true' if present");
		}

		boost::shared_ptr<ListedRequirement> req =
			boost::make_shared<BasicCoreListedRequirement>(
				activity, m_chargingRequirementCore,
				m_chargingRequirementCore->IsMet());

		m_chargingRequirements.push_back(*req);
		return req;
	} else if (name == "docked") {
		if ((value.type() != MojObject::TypeBool) || !value.boolValue()) {
			throw std::runtime_error("A \"docked\" requirement must specify "
				"'true' if present");
		}

		boost::shared_ptr<ListedRequirement> req =
			boost::make_shared<BasicCoreListedRequirement>(
				activity, m_dockedRequirementCore,
				m_dockedRequirementCore->IsMet());

		m_dockedRequirements.push_back(*req);
		return req;
	} else if (name == "battery") {
		if ((value.type() != MojObject::TypeInt) ||
			(value.intValue() < 0) || (value.intValue() > 100)) {
			throw std::runtime_error("A \"battery\" requirement must specify "
				"a value between 0 and 100");
		}

		MojInt64 percent = value.intValue();

		boost::shared_ptr<BatteryRequirement> req =
			boost::make_shared<BatteryRequirement>(activity, percent,
				boost::dynamic_pointer_cast<PowerdProxy,
					RequirementManager>(shared_from_this()),
				(m_batteryPercent >= percent));
		m_batteryRequirements.insert(*req);
		return req;
	} else {
		MojLogError(s_log, _T("[Manager %s] does not know how to instantiate "
			"[Requirement %s] for [Activity %llu]"), GetName().c_str(),
			name.c_str(), activity->GetId());
		throw std::runtime_error("Attempt to instantiate unknown requirement");
	}
}

void PowerdProxy::RegisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	MojLogTrace(s_log);
	MojLogNotice(s_log, _T("Registering requirements"));

	master->RegisterRequirement("charging", shared_from_this());
	master->RegisterRequirement("docked", shared_from_this());
	master->RegisterRequirement("battery", shared_from_this());
}

void PowerdProxy::UnregisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	MojLogTrace(s_log);
	MojLogNotice(s_log, _T("Unregistering requirements"));

	master->UnregisterRequirement("charging", shared_from_this());
	master->UnregisterRequirement("docked", shared_from_this());
	master->UnregisterRequirement("battery", shared_from_this());
}

void PowerdProxy::Enable()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Enabling battery and charging signals"));

	EnableChargerSignals();
	EnableBatterySignals();
}

void PowerdProxy::Disable()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Disabling battery and charging signals"));

	m_chargerStatus.reset();
	m_batteryStatus.reset();

	m_triggerChargerStatus.reset();

	m_chargerStatusSubscribed = false;
	m_batteryStatusSubscribed = false;
}

boost::shared_ptr<PowerActivity> PowerdProxy::CreatePowerActivity(
	boost::shared_ptr<Activity> activity)
{
	return boost::make_shared<PowerdPowerActivity>(
		boost::dynamic_pointer_cast<PowerManager, RequirementManager>
			(shared_from_this()), activity, m_service, m_serial++);
}

MojInt64 PowerdProxy::GetBatteryPercent() const
{
	return m_batteryPercent;
}

void PowerdProxy::TriggerChargerStatus()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Triggering status signals for \"charging\" and "
		"\"docked\" requirements"));

	MojObject params(MojObject::TypeObject);

#if 0
	m_triggerChargerStatus = boost::make_shared<MojoWeakPtrCall<PowerdProxy> >(
		boost::dynamic_pointer_cast<PowerdProxy, RequirementManager>(
			shared_from_this()), &PowerdProxy::TriggerChargerStatusResponse,
		m_service, "palm://com.palm.telephony/chargeSourceQuery", params);
	m_triggerChargerStatus->Call();
#endif

	m_triggerChargerStatus = boost::make_shared<MojoWeakPtrCall<PowerdProxy> >(
		boost::dynamic_pointer_cast<PowerdProxy, RequirementManager>(
			shared_from_this()), &PowerdProxy::TriggerChargerStatusResponse,
		m_service, "palm://com.palm.power/com/palm/power/chargerStatusQuery",
		params);
	m_triggerChargerStatus->Call();
}

void PowerdProxy::TriggerBatteryStatus()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Triggering status signals for \"battery\" "
		"requirement"));

	MojObject params(MojObject::TypeObject);

	m_triggerBatteryStatus = boost::make_shared<MojoWeakPtrCall<PowerdProxy> >(
		boost::dynamic_pointer_cast<PowerdProxy, RequirementManager>(
			shared_from_this()), &PowerdProxy::TriggerBatteryStatusResponse,
		m_service, "palm://com.palm.power/com/palm/power/batteryStatusQuery",
		params);
	m_triggerBatteryStatus->Call();
}

void PowerdProxy::EnableChargerSignals()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Enabling charger signals"));

	MojErr err;
	MojErr errs = MojErrNone;
	MojObject params(MojObject::TypeObject);

	err = params.putString(_T("category"), "/com/palm/power");
	MojErrAccumulate(errs, err);

	err = params.putString(_T("method"), "chargerStatus");
	MojErrAccumulate(errs, err);

	if (errs != MojErrNone) {
		throw std::runtime_error("Error populating arguments for subscription "
			"to charger status");
	}

	m_chargerStatus = boost::make_shared<MojoWeakPtrCall<PowerdProxy> >(
		boost::dynamic_pointer_cast<PowerdProxy, RequirementManager>(
			shared_from_this()), &PowerdProxy::ChargerStatusSignal,
		m_service, "palm://com.palm.lunabus/signal/addmatch", params,
		MojoCall::Unlimited);
	m_chargerStatus->Call();
}

void PowerdProxy::EnableBatterySignals()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Enabling battery signals"));

	MojErr err;
	MojErr errs = MojErrNone;
	MojObject params(MojObject::TypeObject);

	err = params.putString(_T("category"), "/com/palm/power");
	MojErrAccumulate(errs, err);

	err = params.putString(_T("method"), "batteryStatus");
	MojErrAccumulate(errs, err);

	if (errs != MojErrNone) {
		throw std::runtime_error("Error populating arguments for subscription "
			"to battery status");
	}

	m_batteryStatus = boost::make_shared<MojoWeakPtrCall<PowerdProxy> >(
		boost::dynamic_pointer_cast<PowerdProxy, RequirementManager>(
			shared_from_this()), &PowerdProxy::BatteryStatusSignal,
		m_service, "palm://com.palm.lunabus/signal/addmatch", params,
		MojoCall::Unlimited);
	m_batteryStatus->Call();
}

void PowerdProxy::TriggerChargerStatusResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Attempt to trigger charger status signal generated "
		"response: %s"), MojoObjectJson(response).c_str());

	m_triggerChargerStatus.reset();

	if (err != MojErrNone) {
		MojLogWarning(s_log, _T("Failed to trigger charger status signal!"));
		return;
	}
}

void PowerdProxy::TriggerBatteryStatusResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Attempt to trigger battery status signal generated "
		"response: %s"), MojoObjectJson(response).c_str());

	m_triggerBatteryStatus.reset();

	if (err != MojErrNone) {
		MojLogWarning(s_log, _T("Failed to trigger battery status signal!"));
		return;
	}
}

void PowerdProxy::ChargerStatusSignal(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Received charger status signal: %s"),
		MojoObjectJson(response).c_str());

	if (err != MojErrNone) {
		m_chargerStatusSubscribed = false;

		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("Subscription to charger status signal "
				"experienced an uncorrectable failure: %s"),
				MojoObjectJson(response).c_str());
			m_chargerStatus.reset();
		} else {
			MojLogWarning(s_log, _T("Subscription to charger status signal "
				"failed, resubscribing: %s"),
				MojoObjectJson(response).c_str());
			EnableChargerSignals();
		}
		return;
	}

	if (!m_chargerStatusSubscribed) {
		m_chargerStatusSubscribed = true;
		TriggerChargerStatus();
	}

	/* Not all responses will contain charger information... for example,
	 * the initial subscription result. */
	MojString chargerType;
	bool found = false;
	MojErr err2 = response.get(_T("type"), chargerType, found);
	if (err2) {
		MojLogError(s_log, _T("Error %d retrieving charger type from charger "
			"status signal response"), err2);
		return;
	} else if (found) {
		bool connected;
		found = response.get(_T("connected"), connected);
		if (!found) {
			MojLogWarning(s_log, _T("Charger type \"%s\" found, but not "
				"whether or not it's connected"), chargerType.data());
			return;
		}

		if (chargerType == "usb") {
			m_usbChargerConnected = connected;
		} else if (chargerType == "inductive") {
			m_inductiveChargerConnected = connected;
		} else {
			MojLogWarning(s_log, _T("Unknown charger type \"%s\" found"),
				chargerType.data());
			return;
		}

		found = false;
		MojString name;
		err2 = response.get(_T("name"), name, found);
		if (err2) {
			MojLogError(s_log, _T("Error %d retrieving the specific "
				"name of charger of type \"%s\""), err2, chargerType.data());
		} else if (found) {
			if (name == "puck") {
				m_onPuck = connected;
			}
		}

		if (m_onPuck) {
			if (!m_dockedRequirementCore->IsMet()) {
				MojLogNotice(s_log, _T("Device is now docked"));
				m_dockedRequirementCore->Met();
				std::for_each(m_dockedRequirements.begin(),
					m_dockedRequirements.end(),
					boost::mem_fn(&Requirement::Met));
			}
		} else {
			if (m_dockedRequirementCore->IsMet()) {
				MojLogNotice(s_log, _T("Device is no longer docked"));
				m_dockedRequirementCore->Unmet();
				std::for_each(m_dockedRequirements.begin(),
					m_dockedRequirements.end(),
					boost::mem_fn(&Requirement::Unmet));
			}
		}

		if (m_inductiveChargerConnected || m_usbChargerConnected) {
			if (!m_chargingRequirementCore->IsMet()) {
				MojLogNotice(s_log, _T("Device is now charging"));
				m_chargingRequirementCore->Met();
				std::for_each(m_chargingRequirements.begin(),
					m_chargingRequirements.end(),
					boost::mem_fn(&Requirement::Met));
			}
		} else {
			if (m_chargingRequirementCore->IsMet()) {
				MojLogNotice(s_log, _T("Device is no longer charging"));
				m_chargingRequirementCore->Unmet();
				std::for_each(m_chargingRequirements.begin(),
					m_chargingRequirements.end(),
					boost::mem_fn(&Requirement::Unmet));
			}
		}
	}
}

void PowerdProxy::BatteryStatusSignal(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Received battery status signal: %s"),
		MojoObjectJson(response).c_str());

	if (err != MojErrNone) {
		m_batteryStatusSubscribed = false;

		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("Subscription to battery status signal "
				"experienced an uncorrectable failure: %s"),
				MojoObjectJson(response).c_str());
			m_batteryStatus.reset();
		} else {
			MojLogWarning(s_log, _T("Subscription to battery status signal "
				"failed, resubscribing: %s"),
				MojoObjectJson(response).c_str());
			EnableBatterySignals();
		}
		return;
	}

	if (!m_batteryStatusSubscribed) {
		m_batteryStatusSubscribed = true;
		TriggerBatteryStatus();
	}

	MojInt64 batteryPercent;
	bool found = response.get(_T("percent"), batteryPercent);
	if (found) {
		MojInt64 oldBatteryPercent = m_batteryPercent;

		/* Set battery percent first, because as the requirements become met,
		 * Activities may start and events might be generated which include
		 * the state of the associated battery requirement. */
		m_batteryPercent = batteryPercent;

		if (batteryPercent < oldBatteryPercent) {
			BatteryRequirement::MultiTable::iterator newWatermark =
				m_batteryRequirements.upper_bound(batteryPercent,
					BatteryRequirement::KeyComp());

			/* Currently met requirements may wish to generate updates.
			 * Some of those requirements are about to be unmet due to
			 * battery drain.  Do not generate separate udpates. */
			std::for_each(m_batteryRequirements.begin(), newWatermark,
				boost::mem_fn(&Requirement::Updated));

			std::for_each(newWatermark,
				m_batteryRequirements.upper_bound(oldBatteryPercent,
					BatteryRequirement::KeyComp()),
				boost::mem_fn(&Requirement::Unmet));
		} else if (batteryPercent > oldBatteryPercent) {
			BatteryRequirement::MultiTable::iterator oldWatermark =
				m_batteryRequirements.upper_bound(oldBatteryPercent,
					BatteryRequirement::KeyComp());

			/* Some unmet requirements are about to be met.  For the
			 * currently met ones, just generate an update */
			std::for_each(m_batteryRequirements.begin(), oldWatermark,
				boost::mem_fn(&Requirement::Updated));
			std::for_each(oldWatermark,
				m_batteryRequirements.upper_bound(batteryPercent,
					BatteryRequirement::KeyComp()),
				boost::mem_fn(&Requirement::Met));
		}
	}
}

PowerdProxy::BatteryRequirement::BatteryRequirement(
	boost::shared_ptr<Activity> activity, MojInt64 percent,
	boost::shared_ptr<PowerdProxy> powerd, bool met)
	: SortedRequirement<MojInt64, PowerdProxy>(activity, percent, met)
	, m_powerd(powerd)
{
}

PowerdProxy::BatteryRequirement::~BatteryRequirement()
{
}

const std::string& PowerdProxy::BatteryRequirement::GetName() const
{
	static std::string name("battery");
	return name;
}

MojErr PowerdProxy::BatteryRequirement::ToJson(MojObject& rep,
	unsigned flags) const
{
	MojErr err = MojErrNone;

	if (flags & ACTIVITY_JSON_CURRENT) {
		err = rep.put(_T("battery"), m_powerd->GetBatteryPercent());
	} else {
		err = rep.put(_T("battery"), m_key);
	}

	return err;
}

