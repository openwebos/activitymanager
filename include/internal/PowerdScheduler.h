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

#ifndef __ACTIVITYMANAGER_POWERDSCHEDULER_H__
#define __ACTIVITYMANAGER_POWERDSCHEDULER_H__

#include "Scheduler.h"
#include "MojoCall.h"

class PowerdScheduler: public Scheduler
{
public:
	PowerdScheduler(MojService *service);
	virtual ~PowerdScheduler();

	void ScheduledWakeup();

	virtual void Enable();

protected:
	static const char *PowerdWakeupKey;

	virtual void UpdateTimeout(time_t nextWakeup, time_t curTime);
	virtual void CancelTimeout();

	void MonitorSystemTime();

	size_t FormatWakeupTime(time_t wake, char *at, size_t len) const;

	void HandleTimeoutSetResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
	void HandleTimeoutClearResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	void HandleSystemTimeResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	MojService	*m_service;
	boost::shared_ptr<MojoCall>	m_call;
	boost::shared_ptr<MojoCall>	m_systemTimeCall;
};

#endif /* __ACTIVITYMANAGER_POWERDSCHEDULER_H__ */
