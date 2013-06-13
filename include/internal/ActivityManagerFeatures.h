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

#ifndef __ACTIVITYMANAGER_FEATURES_H__
#define __ACTIVITYMANAGER_FEATURES_H__

/* All defines for high level system feature toggles should live in this
 * file. */

/* ****************************************************************** */
/* GENERAL USE FEATURES */
/* ****************************************************************** */

/* Should the Activity Manager give out random IDs?  Normally this should
 * be enabled as the Activity IDs being hard to guess provides a significant
 * amount of security.
 *
 * The lack of predictability can also make testing and debugging more
 * difficult, however, so handing out sequential IDs is also supported.
 */
#if 0
#define ACTIVITYMANAGER_RANDOM_IDS
#endif

/* Should the Activity Manager make its methods available on the public bus,
 * and track whether Activities were created from the public bus in order to
 * restrict any outgoing calls it might make (Triggers) for those Activities
 * to only use the public bus.
 *
 * Until the issues with libmojo and the public bus worked out (i.e. ASSERTION
 * failures in the libmojo code) this will remain disabled.
 */
#if 1
#define ACTIVITYMANAGER_USE_PUBLIC_BUS
#endif

/* Should the Activity Manager require Open webOS database (DB8) to be
 * available with the proper kinds initialized before starting?
 */
#if 0
#define ACTIVITYMANAGER_REQUIRE_DB
#endif

/* Should the power activities backing an Activity be automatically renewed,
 * allowing power to be held on indefinitely?
 */
#if 0
#define ACTIVITYMANAGER_RENEW_POWER_ACTIVITIES
#endif

/* Should the Activity Manager directly launch the configurator to load its
 * Activities, or should it allow upstart (or some other part of the bootup
 * process) to do so?
 */
#if 0
#define ACTIVITYMANAGER_CALL_CONFIGURATOR
#endif

/* ****************************************************************** */
/* DEVELOPMENT FEATURES */
/* ****************************************************************** */

#if 1
#define ACTIVITYMANAGER_DEVELOPER_METHODS
#endif

/* ****************************************************************** */
/* TEST FEATURES */
/* ****************************************************************** */

#endif /* __ACTIVITYMANAGER_FEATURES_H__ */
