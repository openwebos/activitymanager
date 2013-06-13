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

#include "ActivityAutoAssociation.h"

ActivityAutoAssociation::ActivityAutoAssociation(
	boost::shared_ptr<Activity> activity)
	: m_activity(activity)
{
}

ActivityAutoAssociation::~ActivityAutoAssociation()
{
}

boost::shared_ptr<Activity> ActivityAutoAssociation::GetActivity()
{
	return m_activity.lock();
}

boost::shared_ptr<const Activity> ActivityAutoAssociation::GetActivity() const
{
	return m_activity.lock();
}

ActivitySetAutoAssociation::ActivitySetAutoAssociation(
	boost::shared_ptr<Activity> activity)
	: ActivityAutoAssociation(activity)
{
}

ActivitySetAutoAssociation::~ActivitySetAutoAssociation()
{
}

