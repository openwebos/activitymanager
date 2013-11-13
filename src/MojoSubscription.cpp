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

#include "MojoSubscription.h"
#include "Activity.h"
#include "Logging.h"

#include <luna/MojLunaMessage.h>
#include <stdexcept>

MojoSubscription::MojoSubscription(boost::shared_ptr<Activity> activity,
	bool detailedEvents, MojServiceMessage *msg)
	: Subscription(activity, detailedEvents)
	, m_subscriber(msg)
	, m_msg(msg)
{
}

MojoSubscription::~MojoSubscription()
{
}

void MojoSubscription::EnableSubscription()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] Enabling subscription for %s",
		m_activity->GetId(), GetSubscriber().GetString().c_str());

	/* The message will take a reference to to Cancel Handler when it
	 * registers itself with the message. MojRefCountedPtr's reference
	 * count is internal to the object pointed to. */
	MojRefCountedPtr<MojoSubscription::MojoCancelHandler> cancelHandler(
		new MojoSubscription::MojoCancelHandler(shared_from_this(),
			m_msg.get()));
}

Subscriber& MojoSubscription::GetSubscriber()
{
	return m_subscriber;
}

const Subscriber& MojoSubscription::GetSubscriber() const
{
	return m_subscriber;
}

MojErr MojoSubscription::SendEvent(ActivityEvent_t event)
{
	if (m_msg.get()) {
		MojErr err = MojErrNone;
		MojObject eventReply;

		err = eventReply.putString(_T("event"), ActivityEventNames[event]);
		MojErrCheck(err);

		err = eventReply.putInt(_T("activityId"), m_activity->GetId());
		MojErrCheck(err);

		err = eventReply.putBool(MojServiceMessage::ReturnValueKey, true);
		MojErrCheck(err);

		if (m_detailedEvents) {
			MojObject details(MojObject::TypeObject);

			err = m_activity->ActivityInfoToJson(details);
			MojErrCheck(err);

			err = eventReply.put(_T("$activity"), details);
			MojErrCheck(err);
		}

		err = m_msg->reply(eventReply);
		MojErrCheck(err);

		return MojErrNone;
	}

	return MojErrInvalidArg;
}

std::string MojoSubscription::GetSubscriberString(MojServiceMessage *msg)
{
	MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
	if (!lunaMsg) {
		throw std::runtime_error("Can't generate subscriber string from "
			"non-Luna message");
	}

	const char *appId = lunaMsg->appId();
	if (appId) {
		std::string appIdFixed(appId);
		FixAppId(appIdFixed);
		return BusId::GetString(appIdFixed, BusApp);
	} else {
		const char *serviceId = lunaMsg->senderId();
		if (serviceId) {
			return BusId::GetString(serviceId, BusService);
		} else {
			return BusId::GetString(lunaMsg->senderAddress(),
				BusAnon);
		}
	}
}

std::string MojoSubscription::GetSubscriberString(
	MojRefCountedPtr<MojServiceMessage> msg)
{
	return GetSubscriberString(msg.get());
}

BusId MojoSubscription::GetBusId(MojServiceMessage *msg)
{
	MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
	if (!lunaMsg) {
		throw std::runtime_error("Can't generate subscriber string from "
			"non-Luna message");
	}

	const char *appId = lunaMsg->appId();
	if (appId) {
		std::string appIdFixed(appId);
		FixAppId(appIdFixed);
		return BusId(appIdFixed, BusApp);
	} else {
		const char *serviceId = lunaMsg->senderId();
		if (serviceId) {
			return BusId(serviceId, BusService);
		} else {
			return BusId(lunaMsg->senderAddress(),
				BusAnon);
		}
	}
}

BusId MojoSubscription::GetBusId(MojRefCountedPtr<MojServiceMessage> msg)
{
	return GetBusId(msg.get());
}

void MojoSubscription::FixAppId(std::string& appId)
{
	size_t firstSpace = appId.find_first_of(' ');
	if (firstSpace != std::string::npos) {
		appId.resize(firstSpace);
	}
}

void MojoSubscription::HandleCancel()
{
	m_msg.reset();
}

MojoSubscription::MojoCancelHandler::MojoCancelHandler(
	boost::shared_ptr<Subscription> subscription, MojServiceMessage *msg)
	: m_subscription(subscription)
	, m_msg(msg)
	, m_cancelSlot(this, &MojoSubscription::MojoCancelHandler::HandleCancel)
{
	msg->notifyCancel(m_cancelSlot);
}

MojoSubscription::MojoCancelHandler::~MojoCancelHandler()
{
}

MojErr MojoSubscription::MojoCancelHandler::HandleCancel(
	MojServiceMessage *msg)
{
	if (m_msg == msg) {
		m_subscription->HandleCancelWrapper();
		return MojErrNone;
	} else {
		return MojErrInvalidArg;
	}
}

MojoSubscription::MojoSubscriber::MojoSubscriber(MojServiceMessage *msg)
{
	MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
	if (!lunaMsg) {
		throw std::runtime_error("Can't generate subscriber from non-Luna "
			"message");
	}

	const char *appId = lunaMsg->appId();
	if (appId) {
		std::string appIdFixed(appId);
		MojoSubscription::FixAppId(appIdFixed);
		m_id = BusId(appIdFixed, BusApp);
	} else {
		const char *serviceId = lunaMsg->senderId();

		if (serviceId) {
			m_id = BusId(serviceId, BusService);
		} else {
			m_id = BusId(lunaMsg->senderAddress(), BusAnon);
		}
	}
}

MojoSubscription::MojoSubscriber::~MojoSubscriber()
{
}

