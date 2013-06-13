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

#include "ControlGroupManager.h"
#include "ControlGroup.h"

#ifdef MOJ_LINUX
#include <sys/vfs.h>
#include <linux/magic.h>
#endif

#include <cstring>
#include <stdexcept>

ControlGroupManager::ControlGroupManager(const std::string& root,
	boost::shared_ptr<MasterResourceManager> master)
	: ContainerManager(master)
	, m_root(root)
	, m_supported(false)
{
#ifdef MOJ_LINUX
	struct statfs cgroupstat;

	int err = statfs(m_root.c_str(), &cgroupstat);
	if (err) {
		MojLogError(s_log, _T("Error \"%s\" attempting to stat cgroup "
			"filesystem root \"%s\""), strerror(errno), m_root.c_str());
		return;
	}

	if (cgroupstat.f_type != CGROUP_SUPER_MAGIC) {
		MojLogError(s_log, _T("cgroup filesystem root type (%08x) does "
			"not match cgroup magic type (%08x)"), (unsigned)cgroupstat.f_type,
			(unsigned)CGROUP_SUPER_MAGIC);
		return;
	}

	m_supported = true;
#endif
}

ControlGroupManager::~ControlGroupManager()
{
}

boost::shared_ptr<ResourceContainer> ControlGroupManager::CreateContainer(
	const std::string& name)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("Creating [Container %s]"), name.c_str());

	boost::shared_ptr<ControlGroup> controlGroup;

	if (m_supported) {
		controlGroup = boost::make_shared<ControlGroup>(name,
			boost::dynamic_pointer_cast<ControlGroupManager, ResourceManager>
				(shared_from_this()));
	} else {
		controlGroup = boost::make_shared<NullControlGroup>(name,
			boost::dynamic_pointer_cast<ControlGroupManager, ResourceManager>
				(shared_from_this()));
	}

	controlGroup->Init();

	return controlGroup;
}

ActivityPriority_t ControlGroupManager::GetDefaultPriority() const
{
	return ActivityPriorityLow;
}

ActivityPriority_t ControlGroupManager::GetDisabledPriority() const
{
	return ActivityPriorityHigh;
}

const std::string& ControlGroupManager::GetRoot() const
{
	return m_root;
}
