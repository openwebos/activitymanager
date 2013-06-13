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

#ifndef __ACTIVITYMANAGER_POWERACTIVITY_H__
#define __ACTIVITYMANAGER_POWERACTIVITY_H__

#include "Base.h"

#include <boost/intrusive/list.hpp>

class PowerManager;
class Activity;

class PowerActivity : public boost::enable_shared_from_this<PowerActivity>
{
public:
	enum PowerState { PowerLocked, PowerUnlocked, PowerUnknown };

public:
	PowerActivity(boost::shared_ptr<PowerManager> manager,
		boost::shared_ptr<Activity> activity);
	virtual ~PowerActivity();

	virtual PowerState GetPowerState() const = 0;

	virtual void Begin() = 0;
	virtual void End() = 0;

	boost::shared_ptr<PowerManager> GetManager();
	boost::shared_ptr<Activity> GetActivity();
	boost::shared_ptr<const Activity> GetActivity() const;

protected:
	friend class PowerManager;

	typedef boost::intrusive::list_member_hook<
		boost::intrusive::link_mode<
			boost::intrusive::auto_unlink> > ListItem;
	ListItem	m_listItem;

	boost::shared_ptr<PowerManager>	m_manager;
	boost::weak_ptr<Activity>		m_activity;

	static MojLogger	s_log;
};

class NoopPowerActivity : public PowerActivity
{
public:
	NoopPowerActivity(boost::shared_ptr<PowerManager> manager,
		boost::shared_ptr<Activity> activity);
	virtual ~NoopPowerActivity();

	virtual PowerState GetPowerState() const;

	virtual void Begin();
	virtual void End();

protected:
	PowerState	m_state;
};

#endif /* __ACTIVITYMANAGER_POWERACTIVITY_H__ */
