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

#ifndef __ACTIVITYMANAGER_CONTROLGROUPMANAGER_H__
#define __ACTIVITYMANAGER_CONTROLGROUPMANAGER_H__

#include "ContainerManager.h"

class ControlGroupManager : public ContainerManager
{
public:
	ControlGroupManager(const std::string& root,
		boost::shared_ptr<MasterResourceManager> master);
	virtual ~ControlGroupManager();

	virtual boost::shared_ptr<ResourceContainer> CreateContainer(
		const std::string& name);

	virtual ActivityPriority_t GetDefaultPriority() const;
	virtual ActivityPriority_t GetDisabledPriority() const;

	const std::string& GetRoot() const;

protected:
	std::string	m_root;

	bool	m_supported;
};

#endif /* __ACTIVITYMANAGER_CONTROLGROUPMANAGER_H__ */
