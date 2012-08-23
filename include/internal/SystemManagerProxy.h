/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef __ACTIVITYMANAGER_SYSTEMMANAGERPROXY_H__
#define __ACTIVITYMANAGER_SYSTEMMANAGERPROXY_H__

#include "Requirement.h"
#include "RequirementManager.h"

class Activity;
class ActivityManager;
class MojoCall;

class SystemManagerProxy : public RequirementManager
{
public:
	SystemManagerProxy(MojService *service,
		boost::shared_ptr<ActivityManager> am);
	virtual ~SystemManagerProxy();

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
	void BootStatusUpdate(MojServiceMessage *msg, const MojObject& response,
		MojErr err);

	MojService			*m_service;
	boost::shared_ptr<ActivityManager>	m_am;

	boost::shared_ptr<MojoCall>	m_bootstatus;

	boost::shared_ptr<RequirementCore>	m_bootupRequirementCore;
	RequirementList	m_bootupRequirements;

	bool m_bootIssued;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_SYSTEMMANAGERPROXY_H__ */
