/* @@@LICENSE
*
*      Copyright (c) 2009-2013 LG Electronics, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef __ACTIVITYMANAGER_POWERDPROXY_H__
#define __ACTIVITYMANAGER_POWERDPROXY_H__

#include "Base.h"
#include "PowerManager.h"
#include "SortedRequirement.h"

class MojoCall;

class PowerdProxy : public PowerManager
{
public:
	PowerdProxy(MojService *service);
	virtual ~PowerdProxy();

	virtual const std::string& GetName() const;

	virtual boost::shared_ptr<Requirement> InstantiateRequirement(
		boost::shared_ptr<Activity> activity, const std::string& name,
		const MojObject& value);

	virtual void RegisterRequirements(
		boost::shared_ptr<MasterRequirementManager> master);
	virtual void UnregisterRequirements(
		boost::shared_ptr<MasterRequirementManager> master);

	virtual void Enable();
	virtual void Disable();

	virtual boost::shared_ptr<PowerActivity> CreatePowerActivity(
		boost::shared_ptr<Activity> activity);

	MojInt64 GetBatteryPercent() const;

protected:
	void TriggerChargerStatus();
	void TriggerBatteryStatus();

	void EnableChargerSignals();
	void EnableBatterySignals();

	void TriggerChargerStatusResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
	void TriggerBatteryStatusResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	void ChargerStatusSignal(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
	void BatteryStatusSignal(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	boost::shared_ptr<RequirementCore>	m_chargingRequirementCore;
	boost::shared_ptr<RequirementCore>	m_dockedRequirementCore;

	RequirementList m_chargingRequirements;
	RequirementList m_dockedRequirements;

	boost::shared_ptr<MojoCall>	m_triggerChargerStatus;
	boost::shared_ptr<MojoCall>	m_triggerBatteryStatus;

	boost::shared_ptr<MojoCall>	m_chargerStatus;
	boost::shared_ptr<MojoCall>	m_batteryStatus;

	bool	m_chargerStatusSubscribed;
	bool	m_batteryStatusSubscribed;
	bool	m_inductiveChargerConnected;
	bool	m_usbChargerConnected;
	bool	m_onPuck;

	class BatteryRequirement : public SortedRequirement<MojInt64, PowerdProxy>
	{
	public:
		BatteryRequirement(boost::shared_ptr<Activity> activity,
			MojInt64 percent, boost::shared_ptr<PowerdProxy> powerd,
			bool met = false);
		virtual ~BatteryRequirement();

		virtual const std::string& GetName() const;

		virtual MojErr ToJson(MojObject& rep, unsigned flags) const;

	protected:
		boost::shared_ptr<PowerdProxy>	m_powerd;
	};

	BatteryRequirement::MultiTable	m_batteryRequirements;
	MojInt64		m_batteryPercent;

	MojService		*m_service;

	unsigned long	m_serial;
};

#endif /* __ACTIVITYMANAGER_POWERDPROXY_H__ */
