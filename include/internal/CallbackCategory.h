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

#ifndef __ACTIVITYMANAGER_CALLBACKCATEGORY_H__
#define __ACTIVITYMANAGER_CALLBACKCATEGORY_H__

#include "Base.h"

#include <core/MojService.h>
#include <core/MojServiceMessage.h>

#ifdef UNITTEST
#include "TestDriver.h"
#endif

class Scheduler;

class CallbackCategoryHandler : public MojService::CategoryHandler
{
public:
    CallbackCategoryHandler(boost::shared_ptr<Scheduler> scheduler,
		MojService *service);
    virtual ~CallbackCategoryHandler();

    MojErr Init();

protected:
	/* Wake Callback */
	MojErr ScheduledWakeup(MojServiceMessage *msg, MojObject &payload);

#ifdef UNITTEST
	/* Scheduler Test */
	MojErr SchedulerTest(MojServiceMessage *msg, MojObject &payload);

	boost::shared_ptr<TestDriver>	m_testDriver;
#endif

	static MojLogger	s_log;

	static const Method		s_methods[];

	boost::shared_ptr<Scheduler>	m_scheduler;
	MojService	*m_service;
};

#endif /* __ACTIVITYMANAGER_CALLBACKCATEGORY_H__ */
