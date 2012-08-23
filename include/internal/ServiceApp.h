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

#ifndef __ACTIVITYMANAGER_SERVICEAPP_H__
#define __ACTIVITYMANAGER_SERVICEAPP_H__

#include "Base.h"

#include <core/MojGmainReactor.h>
#include <core/MojReactorApp.h>
#include <luna/MojLunaService.h>

class ActivityCategoryHandler;
class CallbackCategoryHandler;
class DevelCategoryHandler;
class TestCategoryHandler;
class ActivityManager;
class MojoTriggerManager;
class MojoJsonConverter;
class Scheduler;
class PersistProxy;
class MasterRequirementManager;
class MasterResourceManager;
class PowerManager;
class ControlGroupManager;
class LunaBusProxy;

class ActivityManagerApp : public MojReactorApp<MojGmainReactor>
{
public:
    ActivityManagerApp();
    virtual ~ActivityManagerApp();

    virtual MojErr open();
	virtual MojErr ready();

protected:
	MojErr online();

	void InitRNG();

	char	m_rngState[256];

private:
    typedef MojReactorApp<MojGmainReactor> Base;

	static const char* const ClientName;
    static const char* const ServiceName;

	boost::shared_ptr<ActivityManager>		m_am;
	boost::shared_ptr<Scheduler>			m_scheduler;
	boost::shared_ptr<MojoTriggerManager>	m_triggerManager;
	boost::shared_ptr<MasterRequirementManager>	m_requirementManager;
	boost::shared_ptr<MasterResourceManager>	m_resourceManager;
	boost::shared_ptr<MojoJsonConverter>	m_json;
	boost::shared_ptr<PersistProxy>			m_db;
	boost::shared_ptr<PowerManager>			m_powerManager;
	boost::shared_ptr<ControlGroupManager>	m_controlGroupManager;
#ifndef TARGET_DESKTOP
	boost::shared_ptr<LunaBusProxy>			m_busProxy;
#endif

    MojRefCountedPtr<ActivityCategoryHandler>	m_handler;
	MojRefCountedPtr<CallbackCategoryHandler>	m_callbackHandler;

#ifdef ACTIVITYMANAGER_DEVELOPER_METHODS
	MojRefCountedPtr<DevelCategoryHandler>		m_develHandler;
#endif

#ifdef UNITTEST
	MojRefCountedPtr<TestCategoryHandler>		m_testHandler;
#endif

	/* The Activity Manager Client is used to handle calls related to the
	 * Activity Manager's operation that don't directly associate with
	 * an Activity.  All the implementation details use that service handle.
	 *
	 * Anything an Activity directly causes - Triggers and Callbacks, and
	 * of course the incoming calls for the Activities and responses to
	 * those calls (and subscriptions) is routed through the main
	 * Activity Manager service on the bus. */
	MojLunaService	m_client;
    MojLunaService	m_service;
};

#endif /* __ACTIVITYMANAGER_SERVICEAPP_H__ */
