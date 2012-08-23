// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "ActivityTypes.h"

const char *ActivityStateNames[] =
{
	"init",			/* ActivityInit			*/
	"waiting",		/* ActivityWaiting		*/
	"blocked",		/* ActivityBlocked		*/
	"queued",		/* ActivityQueued		*/
	"starting",		/* ActivityStarting		*/
	"running",		/* ActivityRunning		*/
	"paused",		/* ActivityPaused		*/
	"complete",		/* ActivityComplete		*/
	"stopping",		/* ActivityStopping		*/
	"stopped",		/* ActivityStopped		*/
	"cancelling",	/* ActivityCancelling	*/
	"cancelled",	/* ActivityCancelled	*/
	"unknown",		/* ActivityUnknown		*/
};

const char *ActivityCommandNames[] =
{
	"null",		/* ActivityNoCommand		*/
	"start",	/* ActivityStartCommand		*/
	"stop",		/* ActivityStopCommand		*/
	"pause",	/* ActivityPauseCommand		*/
	"cancel",	/* ActivityCancelCommand	*/
	"complete",	/* ActivityCompleteCommand	*/
};

const char *ActivityEventNames[] =
{
	"start",	/* ActivityStartEvent		*/
	"stop",		/* ActivityStopEvent		*/
	"pause",	/* ActivityPauseEvent		*/
	"cancel",	/* ActivityCancelEvent		*/
	"complete",	/* ActivityCompleteEvent	*/
	"yield",	/* ActivityYieldEvent		*/
	"orphan",	/* ActivityOrphanEvent		*/
	"adopted",	/* ActivityAdoptedEvent		*/
	"focused",	/* ActivityFocusedEvent		*/
	"unfocused",/* ActivityUnfocusedEvent	*/
	"update"	/* ActivityUpdateEvent		*/
};

const char *ActivityPriorityNames[] =
{
	"none",		/* ActivityPriorityNone		*/
	"lowest",	/* ActivityPriorityLowest	*/
	"low",		/* ActivityPriorityLow		*/
	"normal",	/* ActivityPriorityNormal	*/
	"high",		/* ActivityPriorityHigh		*/
	"highest"	/* ActivityPriorityHighest	*/
};

const ActivityPriority_t ActivityForegroundPriority = ActivityPriorityNormal;
const ActivityPriority_t ActivityBackgroundPriority = ActivityPriorityLow;

