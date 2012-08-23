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

#ifndef __ACTIVITYMANAGER_BASE_H__
#define __ACTIVITYMANAGER_BASE_H__

/* Check to make sure minimum capabilities are present in Boost, and enable */
#include <boost/version.hpp>

#if !defined(BOOST_VERSION) || (BOOST_VERSION < 103900)
#error Boost prior to 1.39.0 contains a critical bug in the smart_ptr class
#endif

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>

/* Boost bind library - defines mem_fn (like std::mem_fun, but can handle
 * smart pointers as well as references).  Also more flexible bind semantics */
#include <boost/mem_fn.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include <stdint.h>

typedef unsigned long long activityId_t;

#include "ActivityManagerFeatures.h"

#include <errno.h>
#include <core/MojString.h>
#include <core/MojErr.h>

#include "MojoObjectWrapper.h"

#endif /* __ACTIVITYMANAGER_BASE_H__ */
