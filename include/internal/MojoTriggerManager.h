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

#ifndef __ACTIVITYMANAGER_TRIGGERMANAGER_H__
#define __ACTIVITYMANAGER_TRIGGERMANAGER_H__

#include "Base.h"

class Activity;
class Trigger;
class MojoMatcher;
class MojoURL;

class MojoTriggerManager {
public:
	MojoTriggerManager(MojService *service);
	virtual ~MojoTriggerManager();

	boost::shared_ptr<Trigger> CreateKeyedTrigger(
		boost::shared_ptr<Activity> activity, const MojoURL& url,
		const MojObject& params, const MojString& key);

	boost::shared_ptr<Trigger> CreateBasicTrigger(
		boost::shared_ptr<Activity> activity, const MojoURL& url,
		const MojObject& params);

	boost::shared_ptr<Trigger> CreateCompareTrigger(
		boost::shared_ptr<Activity> activity, const MojoURL& url,
		const MojObject& params, const MojString& key, const MojObject& value);

	boost::shared_ptr<Trigger> CreateWhereTrigger(
		boost::shared_ptr<Activity> activity, const MojoURL& url,
		const MojObject& params, const MojObject& where);

protected:
	boost::shared_ptr<Trigger> CreateTrigger(
		boost::shared_ptr<Activity> activity, const MojoURL& url,
		const MojObject& params, boost::shared_ptr<MojoMatcher> matcher);

	MojService	*m_service;
};

#endif /* __ACTIVITYMANAGER_TRIGGERMANAGER_H__ */
