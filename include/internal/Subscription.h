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

#ifndef __ACTIVITYMANAGER_SUBSCRIPTION_H__
#define __ACTIVITYMANAGER_SUBSCRIPTION_H__

#include "Base.h"
#include "ActivityTypes.h"

#include <list>
#include <boost/intrusive/set.hpp>

class Activity;
class Subscriber;

class Subscription : public boost::enable_shared_from_this<Subscription>
{
public:
	Subscription(boost::shared_ptr<Activity> activity, bool detailedEvents);
	virtual ~Subscription();

	virtual void EnableSubscription() = 0;

	virtual Subscriber& GetSubscriber() = 0;
	virtual const Subscriber& GetSubscriber() const = 0;

	virtual MojErr SendEvent(ActivityEvent_t event) = 0;

	MojErr QueueEvent(ActivityEvent_t event);

	void Plug();
	void Unplug();
	bool IsPlugged() const;

	void HandleCancelWrapper();

	bool operator<(const Subscription& rhs) const;

private:
	bool operator==(const Subscription& rhs) const;

protected:
	virtual void HandleCancel() = 0;

	friend class Activity;

	typedef boost::intrusive::set_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink> > SetItem;
	SetItem	m_item;

	bool	m_detailedEvents;
	bool	m_plugged;
	std::list<ActivityEvent_t>	m_eventQueue;

	boost::shared_ptr<Activity>		m_activity;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_SUBSCRIPTION_H_ */
