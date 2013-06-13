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

#ifndef __ACTIVITYMANAGER_COMPLETION_H__
#define __ACTIVITYMANAGER_COMPLETION_H__

#include "Base.h"

#include <core/MojServiceMessage.h>

class Activity;

class Completion
{
public:
	Completion();
	virtual ~Completion();

	virtual void Complete(bool succeeded) = 0;
};

class NoopCompletion : public Completion
{
public:
	NoopCompletion();
	virtual ~NoopCompletion();

	virtual void Complete(bool succeeded);
};

template <class T>
class MojoMsgCompletion : public Completion
{
public:
	typedef void (T::* CallbackType)(MojRefCountedPtr<MojServiceMessage> msg,
		const MojObject& payload, boost::shared_ptr<Activity> activity,
		bool succeeded);

	MojoMsgCompletion(T *category, CallbackType callback,
		MojServiceMessage *msg, const MojObject& payload,
		boost::shared_ptr<Activity> activity)
		: m_payload(payload)
		, m_callback(callback)
		, m_category(category)
		, m_msg(msg)
		, m_activity(activity)
	{
	}

	virtual ~MojoMsgCompletion() {}

	virtual void Complete(bool succeeded)
	{
		((*m_category.get()).*m_callback)(m_msg, m_payload, m_activity,
			succeeded);
	}

protected:
	MojObject		m_payload;
	CallbackType	m_callback;

	MojRefCountedPtr<T>		m_category;
	MojRefCountedPtr<MojServiceMessage>	m_msg;

	boost::shared_ptr<Activity>	m_activity;
};

template<class T>
class MojoRefCompletion : public Completion
{
public:
	typedef void (T::* CallbackType)(boost::shared_ptr<Activity> activity,
		bool succeeded);

	MojoRefCompletion(T *target, CallbackType callback,
		boost::shared_ptr<Activity> activity)
		: m_callback(callback)
		, m_target(target)
		, m_activity(activity)
	{
	}

	virtual ~MojoRefCompletion() {}

	virtual void Complete(bool succeeded)
	{
		((*m_target.get()).*m_callback)(m_activity, succeeded);
	}

protected:
	CallbackType				m_callback;
	MojRefCountedPtr<T>			m_target;
	boost::shared_ptr<Activity>	m_activity;
};

class ActivityCompletion : public Completion
{
public:
	typedef void (Activity::* CallbackType)(bool succeeded);

	ActivityCompletion(boost::shared_ptr<Activity> activity,
		CallbackType callback)
		: m_activity(activity)
		, m_callback(callback)
	{
	}

	virtual ~ActivityCompletion() {}

	virtual void Complete(bool succeeded)
	{
		((*m_activity.get()).*m_callback)(succeeded);
	}

protected:
	boost::shared_ptr<Activity>	m_activity;
	CallbackType				m_callback;
};

#endif /* __ACTIVITYMANAGER_COMPLETION_H__ */
