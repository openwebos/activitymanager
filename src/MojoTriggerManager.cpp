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

#include "MojoTriggerManager.h"
#include "MojoTrigger.h"
#include "MojoTriggerSubscription.h"
#include "MojoMatcher.h"

MojoTriggerManager::MojoTriggerManager(MojService *service)
	: m_service(service)
{
}

MojoTriggerManager::~MojoTriggerManager()
{
}

boost::shared_ptr<Trigger> MojoTriggerManager::CreateKeyedTrigger(
	boost::shared_ptr<Activity> activity, const MojoURL& url,
	const MojObject& params, const MojString& key)
{
	boost::shared_ptr<MojoMatcher> matcher =
		boost::make_shared<MojoKeyMatcher>(key);

	return CreateTrigger(activity, url, params, matcher);
}

boost::shared_ptr<Trigger> MojoTriggerManager::CreateBasicTrigger(
	boost::shared_ptr<Activity> activity, const MojoURL& url,
	const MojObject& params)
{
	boost::shared_ptr<MojoMatcher> matcher =
		boost::make_shared<MojoSimpleMatcher>();

	return CreateTrigger(activity, url, params, matcher);
}

boost::shared_ptr<Trigger> MojoTriggerManager::CreateCompareTrigger(
	boost::shared_ptr<Activity> activity, const MojoURL& url,
	const MojObject& params, const MojString& key, const MojObject& value)
{
	boost::shared_ptr<MojoMatcher> matcher =
		boost::make_shared<MojoCompareMatcher>(key, value);

	return CreateTrigger(activity, url, params, matcher);
}

boost::shared_ptr<Trigger> MojoTriggerManager::CreateWhereTrigger(
	boost::shared_ptr<Activity> activity, const MojoURL& url,
	const MojObject& params, const MojObject& where)
{
	boost::shared_ptr<MojoMatcher> matcher =
		boost::make_shared<MojoWhereMatcher>(where);

	return CreateTrigger(activity, url, params, matcher);
}

boost::shared_ptr<Trigger> MojoTriggerManager::CreateTrigger(
	boost::shared_ptr<Activity> activity, const MojoURL& url,
	const MojObject& params, boost::shared_ptr<MojoMatcher> matcher)
{
	boost::shared_ptr<MojoExclusiveTrigger> trigger =
		boost::make_shared<MojoExclusiveTrigger>(activity, matcher);

	boost::shared_ptr<MojoTriggerSubscription> subscription =
		boost::make_shared<MojoExclusiveTriggerSubscription>(trigger,
			m_service, url, params);

	trigger->SetSubscription(subscription);

	return trigger;
}

