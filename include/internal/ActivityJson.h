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

#ifndef __ACTIVITYMANAGER_JSON_H__
#define __ACTIVITYMANAGER_JSON_H__

/* Include subscriber information in the output */
#define ACTIVITY_JSON_SUBSCRIBERS	0x01

/* Include more detailed information */
#define ACTIVITY_JSON_DETAIL		0x02

/* Include information which is intended to be stored in MojoDB */
#define ACTIVITY_JSON_PERSIST		0x04

/* For things that can vary in their status, include the *current* status.
 * Specifically, this will cause requirements to output whatever values are
 * causing them to be met (or not met).
 */
#define ACTIVITY_JSON_CURRENT		0x08

/* Include internal state information for debugging
 * (m_ready, m_scheduled, etc.)
 */
#define ACTIVITY_JSON_INTERNAL		0x10

#endif /* __ACTIVITYMANAGER_JSON_H__ */
