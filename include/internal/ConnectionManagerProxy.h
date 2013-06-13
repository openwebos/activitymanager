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

#ifndef __ACTIVITYMANAGER_CONNECTIONMANAGERPROXY_H__
#define __ACTIVITYMANAGER_CONNECTIONMANAGERPROXY_H__

#include "Requirement.h"
#include "RequirementManager.h"

#include <boost/intrusive/set.hpp>

class MojoCall;

class ConnectionManagerProxy : public RequirementManager
{
public:
	ConnectionManagerProxy(MojService *service);
	virtual ~ConnectionManagerProxy();

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

protected:
	void ConnectionManagerUpdate(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	MojErr UpdateWANStatus(const MojObject& response);
	MojErr UpdateWifiStatus(const MojObject& response);

	int GetConfidence(const MojObject& spec) const;
	int ConfidenceToInt(const MojObject& confidenceObj) const;

	boost::shared_ptr<Requirement> InstantiateConfidenceRequirement(
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<RequirementCore> *confidenceCores,
		RequirementList* confidenceLists, const MojObject& confidenceDesc);
	void UpdateConfidenceRequirements(
		boost::shared_ptr<RequirementCore> *confidenceCores,
		RequirementList *confidenceLists, int confidence);

	MojService	*m_service;

	boost::shared_ptr<MojoCall>	m_call;

	RequirementList	m_internetRequirements;
	RequirementList	m_wifiRequirements;
	RequirementList	m_wanRequirements;

	boost::shared_ptr<RequirementCore>	m_internetRequirementCore;
	boost::shared_ptr<RequirementCore>	m_wifiRequirementCore;
	boost::shared_ptr<RequirementCore>	m_wanRequirementCore;

	static const int ConnectionConfidenceMax = 4;
	static const int ConnectionConfidenceUnknown = -1;

	MojString ConnectionConfidenceUnknownName;
	MojString ConnectionConfidenceNames[ConnectionConfidenceMax];

	int m_internetConfidence;
	int m_wifiConfidence;
	int m_wanConfidence;

	RequirementList m_internetConfidenceRequirements[ConnectionConfidenceMax];
	RequirementList m_wifiConfidenceRequirements[ConnectionConfidenceMax];
	RequirementList m_wanConfidenceRequirements[ConnectionConfidenceMax];

	boost::shared_ptr<RequirementCore>
		m_internetConfidenceCores[ConnectionConfidenceMax];
	boost::shared_ptr<RequirementCore>
		m_wanConfidenceCores[ConnectionConfidenceMax];
	boost::shared_ptr<RequirementCore>
		m_wifiConfidenceCores[ConnectionConfidenceMax];

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_CONNECTIONMANAGERPROXY_H__ */
