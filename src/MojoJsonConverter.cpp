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

#include "MojoJsonConverter.h"
#include "Trigger.h"
#include "MojoTriggerManager.h"
#include "MojoCallback.h"
#include "ActivityManager.h"
#include "Schedule.h"
#include "IntervalSchedule.h"
#include "RequirementManager.h"
#include "PowerManager.h"
#include "Scheduler.h"
#include "BusId.h"
#include "Logging.h"

#include <stdexcept>
#include <ctime>

MojLogger MojoJsonConverter::s_log(_T("activitymanager.json"));

MojoJsonConverter::MojoJsonConverter(MojService *service,
	boost::shared_ptr<ActivityManager> am,
	boost::shared_ptr<Scheduler> scheduler,
	boost::shared_ptr<MojoTriggerManager> triggerManager,
	boost::shared_ptr<RequirementManager> requirementManager,
	boost::shared_ptr<PowerManager> powerManager)
	: m_service(service)
	, m_am(am)
	, m_scheduler(scheduler)
	, m_triggerManager(triggerManager)
	, m_requirementManager(requirementManager)
	, m_powerManager(powerManager)
{
}

MojoJsonConverter::~MojoJsonConverter()
{
}

boost::shared_ptr<Activity> MojoJsonConverter::CreateActivity(
	const MojObject& spec, Activity::BusType bus, bool reload)
{
	MojErr err;

	MojString activityName;
	err = spec.getRequired(_T("name"), activityName);
	if (err)
		throw std::runtime_error("Activity name is required");

	MojString activityDescription;
	err = spec.getRequired(_T("description"), activityDescription);
	if (err)
		throw std::runtime_error("Activity description is required");

	boost::shared_ptr<Activity> act;

	if (reload) {
		MojUInt64 id;
		bool found = false;
		err = spec.get(_T("activityId"), id, found);
		if (err || !found) {
			throw std::runtime_error("If reloading Activities, activityId "
				"must be present");
		}

		MojObject creator;
		found = spec.get(_T("creator"), creator);
		if (!found) {
			throw std::runtime_error("If reloading Activities, creator must "
				"be present");
		}
		
		act = m_am->GetNewActivity((activityId_t)id);

		try {
			act->SetCreator(ProcessBusId(creator));
		} catch (...) {
			LOG_AM_ERROR(MSGID_DECODE_CREATOR_ID_FAILED, 2, PMLOGKFV("activity","%llu",id),
				  PMLOGKS("creator_id",MojoObjectJson(creator).c_str()),
				  "Unable to decode creator id while reloading");
			m_am->ReleaseActivity(act);
			throw;
		}
	} else {
		act = m_am->GetNewActivity();
	}

	act->SetBusType(bus);

	try {
		/* See if there's a private bus access forcing the creator.  If
		 * reloading, the creator is already set (see above). */
		if (!reload) {
			MojObject creator;
			bool found = spec.get(_T("creator"), creator);
			if (found) {
				if (bus == Activity::PrivateBus) {
					act->SetCreator(ProcessBusId(creator));
				} else {
					LOG_AM_ERROR(MSGID_PBUS_CALLER_SETTING_CREATOR, 2, PMLOGKFV("activity","%llu",act->GetId()),
						    PMLOGKS("creator_id",MojoObjectJson(creator).c_str()),
						    "Attempt to set creator by caller from the public bus");
					throw std::runtime_error("Callers from the public bus "
						"may not set the creator");
				}
			}
		}

		act->SetName(activityName.data());
		act->SetDescription(activityDescription.data());

		MojObject metadata;
		if (spec.get(_T("metadata"), metadata)) {
			if (metadata.type() != MojObject::TypeObject) {
				throw std::runtime_error("Object metadata should be set to "
					"a JSON object");
			}

			MojString metadataJson;
			err = metadata.toJson(metadataJson);
			if (err) {
				throw std::runtime_error("Failed to convert provided Activity "
					"metadata object to JSON string");
			}

			act->SetMetadata(metadataJson.data());
		}

		MojObject typeSpec;
		if (spec.get(_T("type"), typeSpec)) {
			ProcessTypeProperty(act, typeSpec);
		}


		MojObject requirementSpec;
		if (spec.get(_T("requirements"), requirementSpec)) {
			RequirementList addedRequirements;
			RequirementNameList removedRequirements;
			ProcessRequirements(act, requirementSpec, removedRequirements,
				addedRequirements);
			UpdateRequirements(act, removedRequirements, addedRequirements);
		}

		MojObject triggerSpec;
		if (spec.get(_T("trigger"), triggerSpec)) {
			boost::shared_ptr<Trigger> trigger = CreateTrigger(act,
				triggerSpec);
			act->SetTrigger(trigger);
		}

		MojObject callbackSpec;
		if (spec.get(_T("callback"), callbackSpec)) {
			boost::shared_ptr<Callback> callback = CreateCallback(
				act, callbackSpec);
			act->SetCallback(callback);
		}

		MojObject scheduleSpec;
		if (spec.get(_T("schedule"), scheduleSpec)) {
			boost::shared_ptr<Schedule> schedule = CreateSchedule(
				act, scheduleSpec);
			act->SetSchedule(schedule);
		}
	} catch (const std::exception& except) {
		LOG_AM_ERROR(MSGID_EXCEPTION_IN_RELOADING_ACTIVITY, 2, PMLOGKFV("activity","%llu",act->GetId()),
			  PMLOGKS("Exception",except.what()), "Unexpected exception reloading Activity from: '%s'",
			  MojoObjectJson(spec).c_str());
		m_am->ReleaseActivity(act);
		throw;
	} catch (...) {
		LOG_AM_ERROR(MSGID_RELOAD_ACTVTY_UNKNWN_EXCPTN, 1, PMLOGKFV("activity","%llu",act->GetId()),
			  "Unknown exception reloading Activity from: %s", MojoObjectJson(spec).c_str());
		m_am->ReleaseActivity(act);
		throw;
	}

	return act;
}

void MojoJsonConverter::UpdateActivity(
	boost::shared_ptr<Activity> act, const MojObject& spec)
{
	/* First, parse and create the update, so it can be applied "atomically"
	 * Then set or reset the Activity sections, as appropriate */
	bool clearTrigger = false;
	boost::shared_ptr<Trigger> trigger;

	MojObject triggerSpec;
	if (spec.get(_T("trigger"), triggerSpec)) {
		if (triggerSpec.type() == MojObject::TypeBool) {
			if (!triggerSpec.boolValue()) {
				clearTrigger = true;
			} else {
				throw std::runtime_error("\"trigger\":false to clear, or valid "
					"trigger property must be specified");
			}
		} else {
			trigger = CreateTrigger(act, triggerSpec);
		}
	}

	bool clearSchedule = false;
	boost::shared_ptr<Schedule> schedule;

	MojObject scheduleSpec;
	if (spec.get(_T("schedule"), scheduleSpec)) {
		if (scheduleSpec.type() == MojObject::TypeBool) {
			if (!scheduleSpec.boolValue()) {
				clearSchedule = true;
			} else {
				throw std::runtime_error("\"schedule\":false to clear, or "
					"valid schedule property must be specified");
			}
		} else {
			schedule = CreateSchedule(act, scheduleSpec);
		}
	}

	boost::shared_ptr<Callback> callback;

	MojObject callbackSpec;
	if (spec.get(_T("callback"), callbackSpec)) {
		if (callbackSpec.type() == MojObject::TypeBool) {
			if (!callbackSpec.boolValue()) {
				LOG_AM_ERROR(MSGID_RM_ACTVTY_CB_ATTEMPT, 1, PMLOGKFV("activity","%llu",act->GetId()),
					    "Attempt to remove callback on Complete");
				throw std::runtime_error("Activity callback may not be "
					"removed by Complete");
			}
		} else {
			callback = CreateCallback(act, callbackSpec);
		}
	}

	/* Allow Activity Metadata to be updated on 'complete'. */
	bool clearMetadata = false;
	bool setMetadata = false;
	MojObject metadata;
	MojString metadataJson;
	if (spec.get(_T("metadata"), metadata)) {
		if (metadata.type() == MojObject::TypeObject) {
			setMetadata = true;
			MojErr err = metadata.toJson(metadataJson);
			if (err) {
				LOG_AM_ERROR(MSGID_DECODE_METADATA_FAIL, 1, PMLOGKFV("activity","%llu",act->GetId()),
					    "Failed to decode metadata to JSON while attempting to update");
				throw std::runtime_error("Failed to decode metadata to json");
			}
		} else if (metadata.type() == MojObject::TypeBool) {
			if (metadata.boolValue()) {
				LOG_AM_ERROR(MSGID_METADATA_UPDATE_TO_NONOBJ_TYPE, 1, PMLOGKFV("activity","%llu",act->GetId()),
					    "Attempt to update metadata to non-object type");
				throw std::runtime_error("Attempt to set metadata to a "
					"non-object type");
			} else {
				clearMetadata = true;
			}
		} else {
			LOG_AM_ERROR(MSGID_METADATA_UPDATE_TO_NONOBJ_TYPE, 1, PMLOGKFV("activity","%llu",act->GetId()),
				    "Attempt to update metadata to non-object type");
			throw std::runtime_error("Activity metadata must be set to an "
				"object if present");
		}
	}

	RequirementList addedRequirements;
	RequirementNameList removedRequirements;
	MojObject requirements;
	if (spec.get(_T("requirements"), requirements)) {
		ProcessRequirements(act, requirements, removedRequirements,
			addedRequirements);
	}

	if (clearTrigger) {
		act->ClearTrigger();
	} else if (trigger) {
		act->SetTrigger(trigger);
	}

	if (clearSchedule) {
		act->ClearSchedule();
	} else if (schedule) {
		act->SetSchedule(schedule);
	}

	if (callback) {
		act->SetCallback(callback);
	}

	if (setMetadata) {
		act->SetMetadata(metadataJson.data());
	} else if (clearMetadata) {
		act->ClearMetadata();
	}

	if (!addedRequirements.empty() || !removedRequirements.empty()) {
		UpdateRequirements(act, removedRequirements, addedRequirements);
	}
}

boost::shared_ptr<Activity> MojoJsonConverter::LookupActivity(
	const MojObject& payload, const BusId& caller)
{
	boost::shared_ptr<Activity> act;

	MojUInt64 id;
	bool found = false;
	MojErr err = payload.get(_T("activityId"), id, found);
	if (err) {
		throw std::runtime_error("Error retrieving activityId of Activity to "
			"operate on");
	} else if (found) {
		act = m_am->GetActivity((activityId_t)id);
	} else {
		MojErr err;
		MojString name;

		err = payload.get(_T("activityName"), name, found);
		if (err != MojErrNone) {
			throw std::runtime_error("Error retrieving activityName of "
				"Activity to operate on");
		} else if (!found) {
			throw std::runtime_error("Activity ID or name not present in "
				"request");
		}

		act = m_am->GetActivity(name.data(), caller);
	}

	return act;
}

boost::shared_ptr<Trigger> MojoJsonConverter::CreateTrigger(
	boost::shared_ptr<Activity> activity, const MojObject& spec)
{
	MojErr err;

	boost::shared_ptr<Trigger> trigger;

	MojString method;
	MojObject params;

	err = spec.getRequired(_T("method"), method);
	if (err) {
		throw std::runtime_error("Method URL for Trigger is required");
	}

	MojoURL url(method);

	/* If no params, no problem. */
	spec.get(_T("params"), params);

	if (spec.contains(_T("where"))) {
		MojObject where;
		spec.get(_T("where"), where);

		trigger = m_triggerManager->CreateWhereTrigger(activity, url,
			params, where);
	} else if (spec.contains(_T("compare"))) {
		MojObject compare;
		spec.get(_T("compare"), compare);

		if (!compare.contains(_T("key")) || !compare.contains(_T("value"))) {
			throw std::runtime_error("Compare Trigger requires key to compare "
				"and value to compare against be specified.");
		}

		MojString key;
		MojObject value;

		compare.getRequired(_T("key"), key);
		compare.get(_T("value"), value);

		trigger = m_triggerManager->CreateCompareTrigger(activity, url,
			params, key, value);
	} else if (spec.contains(_T("key"))) {
		MojString key;
		spec.getRequired(_T("key"), key);

		trigger = m_triggerManager->CreateKeyedTrigger(activity, url,
			params, key);
	} else {
		trigger = m_triggerManager->CreateBasicTrigger(activity, url, params);
	}

	return trigger;
}

boost::shared_ptr<Callback> MojoJsonConverter::CreateCallback(
	boost::shared_ptr<Activity> activity, const MojObject& spec)
{
	MojErr err;

	MojString method;
	MojObject params;

	err = spec.getRequired(_T("method"), method);
	if (err) {
		throw std::runtime_error("Method URL for Callback is required");
	}

	MojoURL url(method);

	/* If no params, no problem. */
	spec.get(_T("params"), params);

	boost::shared_ptr<Callback> callback = boost::make_shared<MojoCallback>(
		activity, m_service, url, params);

	return callback;
}

boost::shared_ptr<Schedule> MojoJsonConverter::CreateSchedule(
	boost::shared_ptr<Activity> activity, const MojObject& spec)
{
	MojErr err;

	MojString startTimeStr;

	bool found = false;
	err = spec.get(_T("start"), startTimeStr, found);
	if (err) {
		throw std::runtime_error("Failed to retreive start time from JSON");
	}

	bool startIsUTC = true;
	time_t startTime = IntervalSchedule::DAY_ONE;
	if (found) {
		startTime = Scheduler::StringToTime(startTimeStr.data(), startIsUTC);
	}

	found = false;
	MojString intervalStr;
	err = spec.get(_T("interval"), intervalStr, found);
	if (err) {
		throw std::runtime_error("Interval must be specified as a string");
	}

	boost::shared_ptr<Schedule> schedule;

	if (found) {
		bool endIsUTC = true;
		time_t endTime = Schedule::UNBOUNDED;

		found = false;
		MojString endTimeStr;
		err = spec.get(_T("end"), endTimeStr, found);
		if (err)  {
			throw std::runtime_error("Error decoding end time string from "
				"schedule specification");
		} else if (found) {
			endTime = Scheduler::StringToTime(endTimeStr.data(), endIsUTC);
		}

		bool precise = false;
		spec.get(_T("precise"), precise);

		bool relative = false;
		spec.get(_T("relative"), relative);

		bool skip = false;
		spec.get(_T("skip"), skip);

		unsigned interval = IntervalSchedule::StringToInterval(
			intervalStr.data(), !precise);

		if (!precise && ((startTime != Schedule::DAY_ONE) ||
			(endTime != Schedule::UNBOUNDED))) {
			throw std::runtime_error("Unless precise time is specified, "
				"time intervals may not specify a start or end time");
		}

		boost::shared_ptr<IntervalSchedule> intervalSchedule;

		if (!precise && relative) {
			throw std::runtime_error("An interval schedule can be specified as "
				"normal, precise, or precise and relative.");
		} else if (relative) {
			intervalSchedule = boost::make_shared<RelativeIntervalSchedule>
				(m_scheduler, activity, startTime, interval, endTime);
		} else if (precise) {
			intervalSchedule = boost::make_shared<PreciseIntervalSchedule>
				(m_scheduler, activity, startTime, interval, endTime);
		} else {
			intervalSchedule = boost::make_shared<IntervalSchedule>
				(m_scheduler, activity, startTime, interval, endTime);
		}

		if (skip) {
			intervalSchedule->SetSkip(skip);
		}

		bool isUTC = false;

		if ((startTime != Schedule::DAY_ONE) &&
			(endTime != Schedule::UNBOUNDED)) {
			if (startIsUTC != endIsUTC) {
				throw std::runtime_error("Start and end time must both be "
					"specified in UTC or local time");
			}

			isUTC = startIsUTC;
		} else if (startTime != Schedule::DAY_ONE) {
			isUTC = startIsUTC;
		} else if (endTime != Schedule::UNBOUNDED) {
			isUTC = endIsUTC;
		}

		if (!isUTC) {
			intervalSchedule->SetLocal(!isUTC);
		}

		found = false;
		MojString lastFinishedStr;
		err = spec.get(_T("lastFinished"), lastFinishedStr, found);
		if (err) {
			throw std::runtime_error("Error decoding end time string from "
				"schedule specification");
		} else if (found) {
			bool lastFinishedIsUTC = true;
			time_t lastFinishedTime =
				Scheduler::StringToTime(lastFinishedStr.data(),
					lastFinishedIsUTC);
			if (isUTC != lastFinishedIsUTC) {
				LOG_AM_DEBUG("Last finished should use the same time format as the other times in the schedule");
			}
			intervalSchedule->SetLastFinishedTime(lastFinishedTime);
		}

		schedule = intervalSchedule;
	} else {
		if (startTime == IntervalSchedule::DAY_ONE) {
			throw std::runtime_error("Non Interval Schedules must specify a "
				"start time");
		}

		schedule = boost::make_shared<Schedule>(m_scheduler, activity,
			startTime);

		if (!startIsUTC) {
			schedule->SetLocal(true);
		}
	}

	bool local;
	found = spec.get(_T("local"), local);
	if (found) {
		schedule->SetLocal(local);
	}

	return schedule;
}

void MojoJsonConverter::ProcessTypeProperty(
	boost::shared_ptr<Activity> activity, const MojObject& type)
{
	bool found;

	bool persist = false;
	found = type.get(_T("persist"), persist);
	if (found) {
		activity->SetPersistent(persist);
	}

	bool expl = false;
	found = type.get(_T("explicit"), expl);
	if (found) {
		activity->SetExplicit(expl);
	}

	bool continuous;
	found = type.get(_T("continuous"), continuous);
	if (found) {
		activity->SetContinuous(continuous);
	}

	bool userInitiated;
	found = type.get(_T("userInitiated"), userInitiated);
	if (found) {
		activity->SetUserInitiated(userInitiated);
	}

	bool powerActivity;
	found = type.get(_T("power"), powerActivity);
	if (found) {
		if (powerActivity) {
			/* The presence of a PowerActivity defines the 
			 * Activity as a power Activity. */
			activity->SetPowerActivity(
				m_powerManager->CreatePowerActivity(activity));
		}
	}

	bool powerDebounce;
	found = type.get(_T("powerDebounce"), powerDebounce);
	if (found) {
		activity->SetPowerDebounce(powerDebounce);
	}

	/* Immediate can be set explicitly, or implied by foreground or
	 * background.  It should only be set once, however.  Same with 
	 * priority.  Ultimately, you should *have* to set them, either
	 * implicitly by setting foreground or background, or explicitly
	 * by setting both. */
	bool immediateSet = false;
	bool prioritySet = false;
	bool useSimpleType = false;

	bool immediate;
	ActivityPriority_t priority = ActivityPriorityNormal;

	bool foreground;
	found = type.get(_T("foreground"), foreground);
	if (found) {
		if (!foreground) {
			throw std::runtime_error("If present, 'foreground' should be "
				"specified as 'true'");
		}

		immediate = true;
		priority = ActivityForegroundPriority;
		immediateSet = true;
		prioritySet = true;
		useSimpleType = true;
	}

	bool background;
	found = type.get(_T("background"), background);
	if (found) {
		if (!background) {
			throw std::runtime_error("If present, 'background' should be "
				"specified as 'true'");
		}

		if (!immediateSet) {
			immediate = false;
			priority = ActivityBackgroundPriority;
			immediateSet = true;
			prioritySet = true;
			useSimpleType = true;
		} else {
			throw std::runtime_error("Only one of 'foreground' or 'background' "
				"should be specified");
		}
	}

	found = type.get(_T("immediate"), immediate);
	if (found) {
		if (!immediateSet) {
			immediateSet = true;
		} else {
			throw std::runtime_error("Only one of 'foreground', 'background', "
				"or 'immediate' should be specified");
		}
	}

	MojErr err;
	MojString priorityStr;
	found = false;
	err = type.get(_T("priority"), priorityStr, found);
	if (err) {
		throw std::runtime_error("An error occurred attempting to access "
			"the 'priority' property of the type");
	} else if (found) {
		if (!prioritySet) {
			for (int i = 0; i < MaxActivityPriority; i++) {
				if (priorityStr == ActivityPriorityNames[i]) {
					priority = (ActivityPriority_t)i;
					prioritySet = true;
					break;
				}
			}

			if (!prioritySet) {
				throw std::runtime_error("Invalid priority specified");
			}
		} else {
			throw std::runtime_error("Only one of 'foreground', 'background', "
				"or 'priority' should be set");
		}
	}

	if (prioritySet) {
		activity->SetPriority(priority);
	}

	if (immediateSet) {
		activity->SetImmediate(immediate);
	}

	if (prioritySet || immediateSet) {
		activity->SetUseSimpleType(useSimpleType);
	}

	/* See if privilege should be dropped.  Warn on attempt at privilege
	 * escalation */
	MojString bus;
	found = false;
	err = type.get(_T("bus"), bus, found);
	if (err) {
		throw std::runtime_error("An error occurred attempting to access "
			"the 'bus' property of the type");
	} else if (found) {
		if (bus == "private") {
			if (activity->GetBusType() == Activity::PublicBus) {
				throw std::runtime_error("Bus type cannot be set to private "
					"by a request from the public bus");
			}
		} else if (bus == "public") {
			activity->SetBusType(Activity::PublicBus);
		} else {
			throw std::runtime_error("Illegal bus type seen");
		}
	}
}

void MojoJsonConverter::ProcessRequirements(
	boost::shared_ptr<Activity> activity, const MojObject& requirements,
	RequirementNameList& removedRequirements,
	RequirementList& addedRequirements)
{
	for (MojObject::ConstIterator iter = requirements.begin();
		iter != requirements.end(); ++iter) {
		if ((iter.value().type() == MojObject::TypeNull) ||
			((iter.value().type() == MojObject::TypeBool) &&
				!iter.value().boolValue())) {
			removedRequirements.push_back(iter.key().data());
		} else {
			try {
				boost::shared_ptr<Requirement> req =
					m_requirementManager->InstantiateRequirement(activity,
						iter.key().data(), *iter);
				addedRequirements.push_back(req);
			} catch (...) {
#ifdef WEBOS_TARGET_MACHINE_IMPL_SIMULATOR
				LOG_AM_WARNING(MSGID_MGR_NOT_FOUND_FOR_REQUIREMENT, 1, PMLOGKS("Requirement",iter.key().data()),
					    "Unable to find Manager for requirement, skipping");
#else
				/* Really should pre-check the requirements to make sure
				 * the names are all valid, so we don't have to throw here. */
				throw;
#endif
			}
		}
	}
}

void MojoJsonConverter::UpdateRequirements(
	boost::shared_ptr<Activity> activity, 
	RequirementNameList& removedRequirements,
	RequirementList& addedRequirements)
{
	/* Add requirements *FIRST*, then remove (so the Activity doesn't
	 * pass through a state where it's runnable because it has no requirements
	 * if what's happening is a requirement update). */
	for (RequirementList::iterator iter = addedRequirements.begin();
		iter != addedRequirements.end(); ++iter) {
		activity->AddRequirement(*iter);
	}

	for (RequirementNameList::iterator iter = removedRequirements.begin();
		iter != removedRequirements.end(); ++iter) {
		activity->RemoveRequirement(*iter);
	}
}


BusId MojoJsonConverter::ProcessBusId(const MojObject& spec)
{
	if (spec.type() == MojObject::TypeObject) {
		if (spec.contains(_T("appId"))) {
			bool found = false;
			MojString appId;
			MojErr err = spec.get(_T("appId"), appId, found);
			if (err || !found) {
				throw std::runtime_error("Error getting appId string from "
					"object");
			}
			return BusId(appId.data(), BusApp);
		} else if (spec.contains(_T("serviceId"))) {
			bool found = false;
			MojString serviceId;
			MojErr err = spec.get(_T("serviceId"), serviceId, found);
			if (err || !found) {
				throw std::runtime_error("Error getting serviceId string from "
					"object");
			}
			return BusId(serviceId.data(), BusService);
		} else if (spec.contains(_T("anonId"))) {
			LOG_AM_WARNING(MSGID_PERSISTED_ANONID_FOUND, 0, "anonId subscriber found persisted in MojoDB");
			bool found = false;
			MojString anonId;
			MojErr err = spec.get(_T("anonId"), anonId, found);
			if (err || !found) {
				throw std::runtime_error("Error getting anonId string from "
					"object");
			}
			return BusId(anonId.data(), BusAnon);
		} else {
			throw std::runtime_error("Neither appId, serviceId, nor anonId was "
				"found in object");
		}
	} else if (spec.type() == MojObject::TypeString) {
		MojString creatorStr;

		MojErr err = spec.stringValue(creatorStr);
		if (err) {
			throw std::runtime_error("Failed to convert from simple object "
				"to string");
		}

		if (creatorStr.startsWith("appId:")) {
			return BusId(creatorStr.data() + 6, BusApp);
		} else if (creatorStr.startsWith("serviceId:")) {
			return BusId(creatorStr.data() + 10, BusService);
		} else if (creatorStr.startsWith("anonId:")) {
			return BusId(creatorStr.data() + 7, BusAnon);
		} else {
			throw std::runtime_error("BusId string did not start with "
				"\"appId:\", \"serviceId:\", or \"anonId:\"");
		}
	} else {
		throw std::runtime_error("Could not convert to BusId - object is "
			"neither a new format object or old format string");
	}
}

