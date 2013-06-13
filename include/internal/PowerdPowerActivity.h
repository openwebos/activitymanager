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

#ifndef __ACTIVITYMANAGER_POWERDPOWERACTIVITY_H__
#define __ACTIVITYMANAGER_POWERDPOWERACTIVITY_H__

#include "PowerActivity.h"
#include "Timeout.h"

class MojoCall;
class PowerManager;

class PowerdPowerActivity : public PowerActivity
{
public:
	PowerdPowerActivity(boost::shared_ptr<PowerManager> manager,
		boost::shared_ptr<Activity> activity, MojService *service,
		unsigned long serial);
	virtual ~PowerdPowerActivity();

	virtual PowerState GetPowerState() const;

	virtual void Begin();
	virtual void End();

	unsigned long GetSerial() const;

protected:
	void PowerLockedNotification(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
	void PowerUnlockedNotification(MojServiceMessage *msg,
		const MojObject& response, MojErr err, bool debounce);
	void PowerUnlockedNotificationNormal(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
	void PowerUnlockedNotificationDebounce(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	void TimeoutNotification();

	std::string GetRemotePowerActivityName() const;

	MojErr CreateRemotePowerActivity(bool debounce = false);
	MojErr RemoveRemotePowerActivity();

	static const int	PowerActivityLockDuration;			/* 300 */
	static const int	PowerActivityLockUpdateInterval;	/* 240 */
	static const int	PowerActivityDebounceDuration;		/* 12 */

	MojService		*m_service;

	unsigned long	m_serial;
	PowerState		m_currentState;
	PowerState		m_targetState;

	boost::shared_ptr<MojoCall>	m_call;
	boost::shared_ptr<Timeout<PowerdPowerActivity> >	m_timeout;
};

#endif /* __ACTIVITYMANAGER_POWERDPOWERACTIVITY_H__ */
