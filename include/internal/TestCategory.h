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

#ifndef __ACTIVITYMANAGER_TESTCATEGORY_H__
#define __ACTIVITYMANAGER_TESTCATEGORY_H__

#include "Base.h"

#include <core/MojService.h>
#include <core/MojServiceMessage.h>

#include <list>

class MojoJsonConverter;
class Activity;

class TestCategoryHandler : public MojService::CategoryHandler
{
public:
    TestCategoryHandler(boost::shared_ptr<MojoJsonConverter> json);
    virtual ~TestCategoryHandler();

    MojErr Init();

protected:
	/* Grab an extra reference to an Activity (to force it to leak) */
	MojErr Leak(MojServiceMessage *msg, MojObject& payload);

	/* Wake Callback */
	MojErr WhereMatchTest(MojServiceMessage *msg, MojObject &payload);

	MojErr LookupActivity(MojServiceMessage *msg, MojObject& payload,
		boost::shared_ptr<Activity>& act);

	static MojLogger	s_log;

	static const Method		s_methods[];

	typedef std::list<boost::shared_ptr<Activity> > ActivityList;

	ActivityList		m_leakedActivities;

	boost::shared_ptr<MojoJsonConverter>	m_json;
};

#endif /* __ACTIVITYMANAGER_TESTCATEGORY_H__ */
