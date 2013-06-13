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

#ifndef __ACTIVITYMANAGER_TESTDRIVER_H__
#define __ACTIVITYMANAGER_TESTDRIVER_H__

#include "Base.h"

class MojoCall;

class TestDriver : public boost::enable_shared_from_this<TestDriver>
{
public:
	TestDriver(MojService *service);
	virtual ~TestDriver();

	void Start(MojObject& response);

protected:
	void HandleAdoptResponse(MojServiceMessage *msg, const MojObject& response,
		MojErr err);
	void HandleCompleteResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	MojInt64	m_activityId;

	boost::shared_ptr<MojoCall>	m_adopt;
	boost::shared_ptr<MojoCall> m_complete;

	MojService	*m_service;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_TESTDRIVER_H__ */
