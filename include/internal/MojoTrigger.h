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

#ifndef __ACTIVITYMANAGER_MOJOTRIGGER_H__
#define __ACTIVITYMANAGER_MOJOTRIGGER_H__

#include "Trigger.h"

#include <list>

class MojoMatcher;
class MojoTriggerSubscription;

class MojoTrigger : public Trigger
{
public:
	MojoTrigger(boost::shared_ptr<MojoMatcher> matcher);
	virtual ~MojoTrigger();

	void SetSubscription(
		boost::shared_ptr<MojoTriggerSubscription> subscription);

	virtual MojErr ToJson(boost::shared_ptr<const Activity> activity,
		MojObject& rep, unsigned flags) const;

	virtual void ProcessResponse(const MojObject& response, MojErr err) = 0;

protected:
	boost::shared_ptr<MojoMatcher>	m_matcher;
	boost::shared_ptr<MojoTriggerSubscription>	m_subscription;

	static MojLogger	s_log;
};

class MojoExclusiveTrigger : public MojoTrigger
{
public:
	MojoExclusiveTrigger(boost::shared_ptr<Activity> activity,
		boost::shared_ptr<MojoMatcher> matcher);
	virtual ~MojoExclusiveTrigger();

	virtual void Arm(boost::shared_ptr<Activity> activity);
	virtual void Disarm(boost::shared_ptr<Activity> activity);

	virtual void Fire();

	virtual bool IsArmed(boost::shared_ptr<const Activity> activity) const;
	virtual bool IsTriggered(boost::shared_ptr<const Activity> activity) const;

	virtual boost::shared_ptr<Activity> GetActivity();
	virtual boost::shared_ptr<const Activity> GetActivity() const;

	virtual void ProcessResponse(const MojObject& response, MojErr err);

	virtual MojErr ToJson(boost::shared_ptr<const Activity> activity,
		MojObject& rep, unsigned flags) const;

protected:
	MojObject	m_response;
	bool		m_triggered;

	boost::weak_ptr<Activity>	m_activity;
};

class MojoSharedTrigger : public MojoTrigger
{
public:
	MojoSharedTrigger();
	virtual ~MojoSharedTrigger();

	void AddActivity(boost::shared_ptr<Activity> activity);
	void RemoveActivity(boost::shared_ptr<Activity> activity);

protected:
	typedef std::list<boost::weak_ptr<Activity> > ActivityList;

	ActivityList m_activities;
};

#endif /* __ACTIVITYMANAGER_MOJOTRIGGER_H__ */
