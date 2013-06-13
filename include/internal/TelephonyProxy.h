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

#ifndef __ACTIVITYMANAGER_TELEPHONYPROXY_H__
#define __ACTIVITYMANAGER_TELEPHONYPROXY_H__

#include "Requirement.h"
#include "RequirementManager.h"

class MojoCall;

class TelephonyProxy : public RequirementManager
{
public:
	TelephonyProxy(MojService *service);
	virtual ~TelephonyProxy();

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
	void NetworkStatusUpdate(MojServiceMessage *msg, const MojObject& reponse,
		MojErr err);
	void PlatformQueryUpdate(MojServiceMessage *msg, const MojObject& response,
	    MojErr err);

private:
	void UpdateTelephonyState();

	MojService			*m_service;
	bool                m_haveTelephonyService;

	boost::shared_ptr<MojoCall>			m_networkStatusQuery;
	boost::shared_ptr<MojoCall>         m_platformQuery;
	boost::shared_ptr<RequirementCore>	m_telephonyRequirementCore;

	RequirementList	m_telephonyRequirements;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_TELEPHONYPROXY_H__ */
