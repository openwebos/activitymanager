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

#ifndef __ACTIVITYMANAGER_LUNABUSPROXY_H__
#define __ACTIVITYMANAGER_LUNABUSPROXY_H__

#include "Base.h"
#include "ContainerManager.h"
#include "BusId.h"

#include <boost/regex.hpp>

#include <set>
#include <list>

class MojoCall;

class LunaBusProxy : public boost::enable_shared_from_this<LunaBusProxy> {
public:
	LunaBusProxy(boost::shared_ptr<ContainerManager> containerManager,
		MojService *service);
	virtual ~LunaBusProxy();

	virtual void Enable();
	virtual void Disable();

protected:
	void EnableBusUpdates();
	void ProcessBusUpdate(MojServiceMessage *msg, const MojObject& response,
		MojErr err);

	void PopulateBusIds(const MojObject& ids,
		ContainerManager::BusIdVec& busIdVec,
		const MojString& serviceName) const;

	bool FilterName(const MojString& name) const;

	void InitializeRestrictedIds();
	bool ContainsRestricted(const ContainerManager::BusIdVec& busIdVec) const;

	/* If any of these names are present in the vector of names, the
	 * service should *NOT* be mapped to a container. */
	typedef std::set<BusId>	RestrictedSet;
	RestrictedSet	m_restrictedIds;

	typedef std::list<boost::regex> RestrictedPatternList;
	RestrictedPatternList	m_restrictedPatterns;

	MojService		*m_service;

	boost::shared_ptr<MojoCall>	m_busUpdates;
	boost::shared_ptr<ContainerManager>	m_containerManager;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_LUNABUSPROXY_H__ */
