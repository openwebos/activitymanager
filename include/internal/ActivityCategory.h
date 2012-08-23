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

#ifndef __ACTIVITYMANAGER_ACTIVITYCATEGORY_H__
#define __ACTIVITYMANAGER_ACTIVITYCATEGORY_H__

#include "Base.h"

#include <core/MojService.h>
#include <core/MojServiceMessage.h>

#include "PersistProxy.h"

class Activity;
class ActivityManager;
class PowerManager;
class MojoTriggerManager;
class MojoJsonConverter;
class MasterResourceManager;
class ContainerManager;
class MojoSubscription;

class ActivityCategoryHandler : public MojService::CategoryHandler
{
public:
    ActivityCategoryHandler(boost::shared_ptr<PersistProxy> db,
		boost::shared_ptr<MojoJsonConverter> json,
		boost::shared_ptr<ActivityManager> am,
		boost::shared_ptr<MojoTriggerManager> triggerManager,
		boost::shared_ptr<PowerManager> powerManager,
		boost::shared_ptr<MasterResourceManager> resourceManager,
		boost::shared_ptr<ContainerManager> containerManager);

    virtual ~ActivityCategoryHandler();

    MojErr Init();

protected:
    /* Lifecycle Methods */
    MojErr CreateActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr JoinActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr MonitorActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr ReleaseActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr AdoptActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr CompleteActivity(MojServiceMessage *msg, MojObject& payload);

    /* Control Methods */
    MojErr ScheduleActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr StartActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr StopActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr CancelActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr PauseActivity(MojServiceMessage *msg, MojObject& payload);

	/* Completion methods */
	void FinishCreateActivity(MojRefCountedPtr<MojServiceMessage> msg,
		const MojObject& payload, boost::shared_ptr<Activity> act,
		bool succeeded);
	void FinishReplaceActivity(boost::shared_ptr<Activity> act,
		bool succeeded);
	void FinishCompleteActivity(MojRefCountedPtr<MojServiceMessage> msg,
		const MojObject& payload, boost::shared_ptr<Activity> act,
		bool succeeded);
	void FinishStopActivity(MojRefCountedPtr<MojServiceMessage> msg,
		const MojObject& payload, boost::shared_ptr<Activity> act,
		bool succeeded);
	void FinishCancelActivity(MojRefCountedPtr<MojServiceMessage> msg,
		const MojObject& payload, boost::shared_ptr<Activity> act,
		bool succeeded);

    /* Focus Control Methods */
    MojErr FocusActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr UnfocusActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr AddFocus(MojServiceMessage *msg, MojObject& payload);

    /* Introspection */
    MojErr ListActivities(MojServiceMessage *msg, MojObject& payload);
    MojErr GetActivityDetails(MojServiceMessage *msg, MojObject& payload);

	/* Membership Methods */
    MojErr AssociateApp(MojServiceMessage *msg, MojObject& payload);
    MojErr DissociateApp(MojServiceMessage *msg, MojObject& payload);

    MojErr AssociateService(MojServiceMessage *msg, MojObject& payload);
    MojErr DissociateService(MojServiceMessage *msg, MojObject& payload);

    MojErr AssociateProcess(MojServiceMessage *msg, MojObject& payload);
    MojErr DissociateProcess(MojServiceMessage *msg, MojObject& payload);

    MojErr AssociateNetworkFlow(MojServiceMessage *msg, MojObject& payload);
    MojErr DissociateNetworkFlow(MojServiceMessage *msg, MojObject& payload);

	/* Mapping Methods */
    MojErr MapProcess(MojServiceMessage *msg, MojObject& payload);
    MojErr UnmapProcess(MojServiceMessage *msg, MojObject& payload);

	/* Debugging/Information Methods */
	MojErr Info(MojServiceMessage *msg, MojObject& payload);

	/* External Enable/Disable of for scheduling new Activities */
	MojErr Enable(MojServiceMessage *msg, MojObject& payload);
	MojErr Disable(MojServiceMessage *msg, MojObject& payload);

	/* Service Methods */
	MojErr LookupActivity(MojServiceMessage *msg, MojObject& payload,
		boost::shared_ptr<Activity>& act);

	bool IsPublicMessage(MojServiceMessage *msg) const;

	bool SubscribeActivity(MojServiceMessage *msg, const MojObject& payload,
		const boost::shared_ptr<Activity>& act,
		boost::shared_ptr<MojoSubscription>& sub);

	MojErr CheckSerial(MojServiceMessage *msg, const MojObject& payload,
		const boost::shared_ptr<Activity>& act);

	typedef void (ActivityCategoryHandler::* CompletionFunction)(
		MojRefCountedPtr<MojServiceMessage> msg, const MojObject& payload,
		boost::shared_ptr<Activity> act, bool succeeded);

	void EnsureCompletion(MojServiceMessage *msg, MojObject& payload,
		PersistProxy::CommandType type, CompletionFunction func,
		boost::shared_ptr<Activity> act);
	void EnsureReplaceCompletion(MojServiceMessage *msg, MojObject& payload,
		boost::shared_ptr<Activity> oldActivity,
		boost::shared_ptr<Activity> newActivity);

	static MojLogger	s_log;

	static const Method		s_methods[];

	boost::shared_ptr<PersistProxy>			m_db;
	boost::shared_ptr<MojoJsonConverter>	m_json;
	boost::shared_ptr<ActivityManager>		m_am;
	boost::shared_ptr<MojoTriggerManager>	m_triggerManager;
	boost::shared_ptr<PowerManager>			m_powerManager;
	boost::shared_ptr<MasterResourceManager>	m_resourceManager;
	boost::shared_ptr<ContainerManager>		m_containerManager;
};

#endif /* __ACTIVITYMANAGER_ACTIVITYCATEGORY_H__ */
