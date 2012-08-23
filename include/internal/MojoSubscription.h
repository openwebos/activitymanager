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

#ifndef __ACTIVITYMANAGER_MOJOSUBSCRIPTION_H__
#define __ACTIVITYMANAGER_MOJOSUBSCRIPTION_H_

#include "BusId.h"
#include "Subscription.h"
#include "Subscriber.h"

#include <core/MojService.h>
#include <core/MojServiceMessage.h>

/* This wraps the libmojo/LS2 communications abstraction.  A MojoSubscription
 * will exist until cancelled from within the LS2 layer (the MojSignalHandler
 * maintains the reference, so live MojoSubscriptions can maintain the
 * reference counts of other objects - such as Activities)
 */

class MojoSubscription : public Subscription
{
public:
	MojoSubscription(boost::shared_ptr<Activity> activity, bool detailedEvents,
		MojServiceMessage *msg);
	virtual ~MojoSubscription();

	virtual void EnableSubscription();

	virtual MojErr SendEvent(ActivityEvent_t event);

	Subscriber& GetSubscriber();
	const Subscriber& GetSubscriber() const;

	static std::string GetSubscriberString(MojServiceMessage *msg);
	static std::string GetSubscriberString(
		MojRefCountedPtr<MojServiceMessage> msg);
	static BusId GetBusId(MojServiceMessage *msg);
	static BusId GetBusId(MojRefCountedPtr<MojServiceMessage> msg);

	static void FixAppId(std::string& appId);

protected:
	virtual void HandleCancel();

	class MojoCancelHandler : public MojSignalHandler
	{
	public:
		MojoCancelHandler(boost::shared_ptr<Subscription> subscription,
			MojServiceMessage *msg);
		virtual ~MojoCancelHandler();

	private:
		MojErr HandleCancel(MojServiceMessage *msg);

		boost::shared_ptr<Subscription>	m_subscription;
		MojServiceMessage	*m_msg;
		MojServiceMessage::CancelSignal::Slot<MojoCancelHandler> m_cancelSlot;
	};

	class MojoSubscriber : public Subscriber
	{
	public:
		MojoSubscriber(MojServiceMessage *msg);
		virtual ~MojoSubscriber();
	};

	MojoSubscriber m_subscriber;
	MojRefCountedPtr<MojServiceMessage>	m_msg;
};

#endif /* __ACTIVITYMANAGER_MOJOSUBSCRIPTION_H_ */
