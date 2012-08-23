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

#ifndef __ACTIVITYMANAGER_DEVELCATEGORY_H__
#define __ACTIVITYMANAGER_DEVELCATEGORY_H__

#include "Base.h"

#include <core/MojService.h>
#include <core/MojServiceMessage.h>

class ActivityManager;
class MojoJsonConverter;
class MasterResourceManager;
class ContainerManager;
class Activity;

class DevelCategoryHandler : public MojService::CategoryHandler
{
public:
    DevelCategoryHandler(boost::shared_ptr<ActivityManager> am,
		boost::shared_ptr<MojoJsonConverter> json,
		boost::shared_ptr<MasterResourceManager> resourceManager,
		boost::shared_ptr<ContainerManager> containerManager);
    virtual ~DevelCategoryHandler();

    MojErr Init();

protected:
	/* Boot Activity from the Background Queue */
	MojErr Evict(MojServiceMessage *msg, MojObject& payload);

	/* Run an Activity (or all Activities) */
	MojErr Run(MojServiceMessage *msg, MojObject& payload);

	/* Set background Activity concurrency level for the Activity Manager */
	MojErr SetConcurrency(MojServiceMessage *msg, MojObject& payload);

	/* Enable or disable priority control */
	MojErr PriorityControl(MojServiceMessage *msg, MojObject& payload);

	/* Map processes into containers */
	MojErr MapProcess(MojServiceMessage *msg, MojObject& payload);

	MojErr LookupActivity(MojServiceMessage *msg, MojObject& payload,
		boost::shared_ptr<Activity>& act);

	static MojLogger	s_log;

	static const Method		s_methods[];

	boost::shared_ptr<ActivityManager>			m_am;
	boost::shared_ptr<MojoJsonConverter>		m_json;
	boost::shared_ptr<MasterResourceManager>	m_resourceManager;
	boost::shared_ptr<ContainerManager>			m_containerManager;
};

#endif /* __ACTIVITYMANAGER_DEVELCATEGORY_H__ */
