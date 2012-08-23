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

#ifndef __ACTIVITYMANAGER_CONTROLGROUP_H__
#define __ACTIVITYMANAGER_CONTROLGROUP_H__

#include "ResourceContainer.h"
#include <list>

class ControlGroup : public ResourceContainer
{
public:
	ControlGroup(const std::string& name,
		boost::shared_ptr<ContainerManager> manager);
	virtual ~ControlGroup();

	void Init();

	virtual void UpdatePriority();
	virtual void MapProcess(pid_t pid);

	virtual void Enable();
	virtual void Disable();

	virtual MojErr ToJson(MojObject& rep) const;

protected:
	virtual void SetPriority(ActivityPriority_t priority, bool focused);

	const std::string& GetRoot() const;

	virtual bool WriteControlFile(const std::string& controlFile,
		const char *buf, size_t count, std::list<int>::iterator current);

	ActivityPriority_t	m_currentPriority;
	bool m_focused;

	// list of processes attatched to this container
	std::list<int> m_processIds;

};

class NullControlGroup : public ControlGroup
{
public:
	NullControlGroup(const std::string& name,
		boost::shared_ptr<ContainerManager> manager);
	virtual ~NullControlGroup();

protected:
	virtual bool WriteControlFile(const std::string& controlFile,
                const char *buf, size_t count, std::list<int>::iterator current);

};

#endif /* __ACTIVITYMANAGER_CONTROLGROUP_H__ */
