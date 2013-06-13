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

#ifndef __ACTIVITYMANAGER_ACTIVITYTYPES_H__
#define __ACTIVITYMANAGER_ACTIVITYTYPES_H__

typedef enum ActivityState_e {
	ActivityInit,		/* "init"		*/
	ActivityWaiting,	/* "waiting"	*/
	ActivityBlocked,	/* "blocked"	*/
	ActivityQueued,		/* "queued"		*/
	ActivityStarting,	/* "starting"	*/
	ActivityRunning,	/* "running"	*/
	ActivityPaused,		/* "paused"		*/
	ActivityComplete,	/* "complete"	*/
	ActivityStopping,	/* "stopping"	*/
	ActivityStopped,	/* "stopped"	*/
	ActivityCancelling,	/* "cancelling"	*/
	ActivityCancelled,	/* "cancelled"	*/
	ActivityUnknown,	/* "unknown"	*/
	MaxActivityState
} ActivityState_t;

extern const char *ActivityStateNames[];

typedef enum ActivityCommand_e {
	ActivityNoCommand,
	ActivityStartCommand,
	ActivityStopCommand,
	ActivityPauseCommand,
	ActivityCancelCommand,
	ActivityCompleteCommand,
	MaxCommandState
} ActivityCommand_t;

extern const char *ActivityCommandNames[];

typedef enum ActivityEvent_e {
	/* Commands */
	ActivityStartEvent,		/* "start"		*/
	ActivityStopEvent,		/* "stop"		*/
	ActivityPauseEvent,		/* "pause"		*/
	ActivityCancelEvent,	/* "cancel"		*/
	ActivityCompleteEvent,	/* "complete"	*/
	ActivityYieldEvent,		/* "yield"		*/

	/* Events */
	ActivityOrphanEvent,	/* "orphan" 	*/
	ActivityAdoptedEvent,	/* "adopted"	*/
	ActivityFocusedEvent,	/* "focused"	*/
	ActivityUnfocusedEvent,	/* "unfocused"	*/
	ActivityUpdateEvent,	/* "update"		*/
	MaxActivityEvent
} ActivityEvent_t;

extern const char *ActivityEventNames[];

typedef enum ActivityPriority_e {
	ActivityPriorityNone,
	ActivityPriorityLowest,
	ActivityPriorityLow,
	ActivityPriorityNormal,
	ActivityPriorityHigh,
	ActivityPriorityHighest,
	MaxActivityPriority
} ActivityPriority_t;

extern const char *ActivityPriorityNames[];

extern const ActivityPriority_t	ActivityForegroundPriority;
extern const ActivityPriority_t	ActivityBackgroundPriority;

#endif /* __ACTIVITYMANAGER_ACTIVITYTYPES_H__ */
