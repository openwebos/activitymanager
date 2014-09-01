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

#include "ServiceApp.h"
#include "ActivityCategory.h"
#include "CallbackCategory.h"
#include "ActivityManager.h"
#include "MojoTriggerManager.h"
#include "MojoJsonConverter.h"
#include "Scheduler.h"
#include "GlibScheduler.h"
#include "PowerdScheduler.h"
#include "MojoDBProxy.h"
#include "RequirementManager.h"
#include "DefaultRequirementManager.h"
#include "ConnectionManagerProxy.h"
#include "TelephonyProxy.h"
#include "SystemManagerProxy.h"
#include "PowerdProxy.h"
#include "ResourceManager.h"
#include "ControlGroupManager.h"
#include "LunaBusProxy.h"
#include <nyx/nyx_client.h>
#include <glib.h>

#ifdef ACTIVITYMANAGER_DEVELOPER_METHODS
#include "DevelCategory.h"
#endif

#ifdef UNITTEST
#include "TestCategory.h"
#endif

#include <core/MojLogEngine.h>

#include <cstdlib>
#include <ctime>

#ifndef WEBOS_TARGET_MACHINE_IMPL_SIMULATOR
#include <fstream>
#include <cstring>
#include <openssl/sha.h>
#endif
#include "Logging.h"

const char* const ActivityManagerApp::ClientName =
	"com.palm.activitymanager.client";
const char* const ActivityManagerApp::ServiceName = "com.palm.activitymanager";

static long original_timezone_offset;

static void switch_to_utc()
{
	/* Make sure the time globals are set */
	tzset();

	/* Determine current timezone.  Whether to use daylight time can be
	 * found in the 'daylight' global.  (glibc isn't computing it automatically
	 * if you set tm_isdst to -1.  Ouch.) */

	/* Time can shift backwards or forwards... put 24 hours on the clock! */
	time_t base_local_time = 24*60*60;

	struct tm utctm;
	memset(&utctm, 0, sizeof(tm));

	/* No conversions... mktime will do that */
	gmtime_r(&base_local_time, &utctm);

	/* -1 does not appear to work under test, but daylight is set properly */
	utctm.tm_isdst = daylight;

	/* When local time was 'base_local_time', UTC was... */
	time_t utc_time = mktime(&utctm);

	/* Store original timezone offset */
	original_timezone_offset = (base_local_time - utc_time);

	/* Now set time processing to happen in UTC. */
	/* Since almost everything already is, this basically just fixes
	 * mktime(3) */
	setenv("TZ", "", 1);
	tzset();
}

int main(int argc, char** argv)
{
	switch_to_utc();

    ActivityManagerApp app;
    int mainResult = app.main(argc, argv);

    return mainResult;
}
 
ActivityManagerApp::ActivityManagerApp()
#ifdef ACTIVITYMANAGER_USE_PUBLIC_BUS
	: m_client(true)
	, m_service(true)
#endif
{
}

ActivityManagerApp::~ActivityManagerApp()
{
}
static bool read_modem_present()
{
    // read the modem present using Nyx
    nyx_error_t error = NYX_ERROR_GENERIC;
    nyx_device_handle_t device = NULL;
    const char *modem_present;
    bool ismodem_present=true;

    error = nyx_init();
    if (NYX_ERROR_NONE == error)
    {
        error = nyx_device_open(NYX_DEVICE_DEVICE_INFO, "Main", &device);

        if (NYX_ERROR_NONE == error && NULL != device)
        {
            error = nyx_device_info_query(device, NYX_DEVICE_INFO_MODEM_PRESENT, &modem_present);

            if (NYX_ERROR_NONE == error)
            {
                if(g_strcmp0(modem_present,"N")==0)
                {
                    ismodem_present=false;
                }

            }

            nyx_device_close(device);
        }
        nyx_deinit();
    }
    return ismodem_present;
}
MojErr ActivityManagerApp::open()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("%s initializing", name().data());

	InitRNG();

    MojErr err = Base::open();
    MojErrCheck(err);

	// Bring the Activity Manager client online
    err = m_client.open(ClientName);
    MojErrCheck(err);
    err = m_client.attach(m_reactor.impl());
    MojErrCheck(err);

	try {
		m_resourceManager = boost::make_shared<MasterResourceManager>();
#ifndef WEBOS_TARGET_MACHINE_IMPL_SIMULATOR
		m_controlGroupManager = boost::make_shared<ControlGroupManager>(
			"/sys/fs/cgroup/cpuset", m_resourceManager);
		m_resourceManager->SetManager("cpu", m_controlGroupManager);
		m_busProxy = boost::make_shared<LunaBusProxy>(m_controlGroupManager,
			&m_client);
#endif

		m_am = boost::make_shared<ActivityManager>(m_resourceManager);

		m_requirementManager = boost::make_shared<MasterRequirementManager>();
		boost::shared_ptr<DefaultRequirementManager> defaultRequirementManager =
			boost::make_shared<DefaultRequirementManager>();
		m_requirementManager->AddManager(defaultRequirementManager);

#ifdef WEBOS_TARGET_MACHINE_IMPL_SIMULATOR
		m_scheduler = boost::make_shared<GlibScheduler>();
		m_scheduler->SetLocalOffset(original_timezone_offset);
#else
		m_scheduler = boost::make_shared<PowerdScheduler>(&m_client);
#endif

#ifdef WEBOS_TARGET_MACHINE_IMPL_SIMULATOR
		m_powerManager = boost::make_shared<NoopPowerManager>();
#else
		m_powerManager = boost::make_shared<PowerdProxy>(&m_client);
#endif
		m_requirementManager->AddManager(m_powerManager);

		m_triggerManager = boost::make_shared<MojoTriggerManager>(&m_service);
		m_json = boost::make_shared<MojoJsonConverter>(&m_service, m_am,
			m_scheduler, m_triggerManager, m_requirementManager,
			m_powerManager);
		m_db = boost::make_shared<MojoDBProxy>(this, &m_client, m_am, m_json);

#ifndef WEBOS_TARGET_MACHINE_IMPL_SIMULATOR
		boost::shared_ptr<ConnectionManagerProxy> cmp =
			boost::make_shared<ConnectionManagerProxy>(&m_client);
		m_requirementManager->AddManager(cmp);

		boost::shared_ptr<SystemManagerProxy> smp =
			boost::make_shared<SystemManagerProxy>(&m_client, m_am);
		m_requirementManager->AddManager(smp);

               if(read_modem_present())
               {
                   boost::shared_ptr<TelephonyProxy> tp =
                   boost::make_shared<TelephonyProxy>(&m_client);
                   m_requirementManager->AddManager(tp);
               }
#endif
	} catch (...) {
		return MojErrNoMem;
	}

	LOG_AM_DEBUG("%s initialized", name().data());

	/* System is initialized.  All object managers are prepared to instantiate
	 * their objects as Activities are loaded from the database.  Begin
	 * deserializing persistent Activities. */
	m_db->LoadActivities();

	return MojErrNone;
}

MojErr ActivityManagerApp::ready()
{
	LOG_AM_DEBUG("%s ready to accept incoming requests",
		name().data());

	/* All stored Activities have been deserialized from the database.  All
	 * previously persisted Activity IDs are marked as used.  It's safe to
	 * accept requests.  Bring up the Service side interface! */
	MojErr err = online();
	MojErrCheck(err);

	/* Activity Manager prepared to accept requests at this point, but not
	 * to allow Activities to move into the scheduled state where their
	 * outcalls are made. */

#ifndef WEBOS_TARGET_MACHINE_IMPL_SIMULATOR
	/* Engage the Luna Bus Proxy, start mapping services */
	m_busProxy->Enable();
#endif

	/* Start up the various requirement management proxy connections now,
	 * so they don't start a storm of requests once the UI is up. */
	m_requirementManager->Enable();

	/* Subscribe to timezone notifications. */
	m_scheduler->Enable();

#if !defined(WEBOS_TARGET_MACHINE_IMPL_SIMULATOR)
	char *upstart_job = getenv("UPSTART_JOB");
	if (upstart_job) {
		char *upstart_event = g_strdup_printf("/sbin/initctl emit %s-ready",
			upstart_job);
		if (upstart_event) {
			int retVal = ::system(upstart_event);
			if (retVal == -1) {
				LOG_AM_ERROR(MSGID_UPSTART_EMIT_FAIL,0,
					"ServiceApp: Failed to emit upstart event");
			}
			g_free(upstart_event);
		} else {
			LOG_AM_ERROR(MSGID_UPSTART_EMIT_ALLOC_FAIL,0,
				"ServiceApp: Failed to allocate memory for upstart emit");
		}
	}
#endif

#if defined(WEBOS_TARGET_MACHINE_IMPL_SIMULATOR)
	/* SystemManagerProxy not instantiated, so mark the UI as enabled */
	m_am->Enable(ActivityManager::UI_ENABLE);
#endif

    return MojErrNone;
}

void ActivityManagerApp::InitRNG()
{
	memset(m_rngState, 0, sizeof(m_rngState));

	bool initialized = false;

	unsigned int rngSeed;

	if (!initialized) {
		struct timeval tv;
		gettimeofday(&tv, NULL);

		rngSeed = (unsigned)(tv.tv_sec ^ tv.tv_usec);

		initialized = true;

		LOG_AM_DEBUG("Seeding RNG using time: sec %u, usec %u, seed %u",
			(unsigned)tv.tv_sec, (unsigned)tv.tv_usec, rngSeed);
	}

	char *oldstate = initstate(rngSeed, m_rngState, sizeof(m_rngState));

	if (!oldstate) {
		LOG_AM_ERROR(MSGID_INIT_RNG_FAIL, 0, "Failed to initialize the RNG state");
	}
}

MojErr ActivityManagerApp::online()
{
	// Bring the Activity Manager online
	MojErr err = m_service.open(ServiceName);
	MojErrCheck(err);
	err = m_service.attach(m_reactor.impl());
	MojErrCheck(err);

	/* Initialize main call handler:
	 *	palm://com.palm.activitymanager/... */
	m_handler.reset(new ActivityCategoryHandler(m_db, m_json, m_am,
		m_triggerManager, m_powerManager, m_resourceManager,
		m_controlGroupManager));
	MojAllocCheck(m_handler.get());

	err = m_handler->Init();
	MojErrCheck(err);

	err = m_service.addCategory(MojLunaService::DefaultCategory,
		m_handler.get());
	MojErrCheck(err);

	/* Initialize callback category handler:
	 *	palm://com.palm.activitymanager/callback/... */
	m_callbackHandler.reset(new CallbackCategoryHandler(m_scheduler,
		&m_service));
	MojAllocCheck(m_callbackHandler.get());

	err = m_callbackHandler->Init();
	MojErrCheck(err);

	err = m_service.addCategory("/callback", m_callbackHandler.get());
	MojErrCheck(err);

#ifdef ACTIVITYMANAGER_DEVELOPER_METHODS
	/* Initialize developer interface handler:
	 *  palm://com.palm.activitymanager/devel/... */

	m_develHandler.reset(new DevelCategoryHandler(m_am, m_json,
		m_resourceManager, m_controlGroupManager));

	MojAllocCheck(m_develHandler.get());

	err = m_develHandler->Init();
	MojErrCheck(err);

	err = m_service.addCategory("/devel", m_develHandler.get());
	MojErrCheck(err);
#endif

#ifdef UNITTEST
	m_testHandler.reset(new TestCategoryHandler(m_json));
	MojAllocCheck(m_testHandler.get());

	err = m_testHandler->Init();
	MojErrCheck(err);

	err = m_service.addCategory("/test", m_testHandler.get());
	MojErrCheck(err);
#endif

	return MojErrNone;
}

