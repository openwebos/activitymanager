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

#include <stdexcept>
#include <luna/MojLunaMessage.h>

#include "MojoCall.h"
#include "Activity.h"
#include "Logging.h"

const MojUInt32 MojoCall::Unlimited = MojServiceRequest::Unlimited;

unsigned MojoCall::s_serial = 0;

MojLogger MojoCall::s_log("activitymanager.call");

MojoCall::MojoCall(MojService *service,
	const MojoURL& url, const MojObject& params, MojUInt32 replies)
	: m_service(service)
	, m_url(url)
	, m_params(params)
	, m_replies(replies)
	, m_subSerial(0)
{
	m_serial = s_serial++;
}

MojoCall::~MojoCall()
{
	LOG_AM_DEBUG("[Call %u] Cleaning up", m_serial);

	if (m_handler.get()) {
		m_handler->Cancel();
	}
}

MojErr MojoCall::Call()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	return Call(false, NULL);
}

MojErr MojoCall::Call(boost::shared_ptr<Activity> activity)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	return Call(activity->GetBusType() == Activity::PublicBus,
		activity->GetCreator().GetId().c_str());
}

MojErr MojoCall::Call(bool usePublicBus, const char *proxyRequester)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);

	if (proxyRequester) {
		LOG_AM_DEBUG("[Call %u] %s(Proxy for %s) Calling %s %s",
			m_serial, usePublicBus ? "(Public) " : "",
			proxyRequester, m_url.GetString().c_str(),
			MojoObjectJson(m_params).c_str());
	} else {
		LOG_AM_DEBUG("[Call %u] %sCalling %s %s", m_serial,
			usePublicBus ? "(Public) " : "",
			m_url.GetString().c_str(), MojoObjectJson(m_params).c_str());
	}

	if (!m_service) {
		throw std::runtime_error("Service for Call has not yet been "
			"initialized");
	}

	MojErr err;

	if (m_handler.get()) {
		m_handler->Cancel();
		m_handler.reset();
	}

	MojRefCountedPtr<MojServiceRequest> req;

	MojLunaService *service = dynamic_cast<MojLunaService *>(m_service);
	if (!service) {
		throw std::runtime_error("Service for call is not a Luna Service");
	}

	if (proxyRequester) {
		err = service->createRequest(req, usePublicBus, proxyRequester);
		MojErrCheck(err);
	} else {
		err = service->createRequest(req, usePublicBus);
		MojErrCheck(err);
	}

	MojRefCountedPtr<MojoCall::MojoCallMessageHandler> handler(
		new MojoCallMessageHandler(shared_from_this(), m_subSerial++));
	MojAllocCheck(handler.get());

	err = req->send(handler->GetResponseSlot(), m_url.GetTargetService(),
		m_url.GetMethod(), m_params, m_replies);
	MojErrCheck(err);

	/* Store handler for later cancel... */
	m_handler = handler;

	return MojErrNone;
}

void MojoCall::Cancel()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Call %u] Cancelling call %s", m_serial,
		m_url.GetString().c_str());

	if (m_handler.get()) {
		m_handler->Cancel();
		m_handler.reset();
	}
}

unsigned MojoCall::GetSerial() const
{
	return m_serial;
}

/* Ensure Standardized logging and exception protection are in place */
void MojoCall::HandleResponseWrapper(MojServiceMessage *msg, MojObject& response, MojErr err)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Call %u] %s: Received response %s", m_serial,
		m_url.GetString().c_str(), MojoObjectJson(response).c_str());

	/* XXX If response count reached, cancel call. */
	try {
		HandleResponse(msg, response, err);
	} catch (const std::exception& except) {
		LOG_AM_ERROR(MSGID_CALL_RESP_UNHANDLED_EXCEPTION, 3, PMLOGKFV("serial","%u",m_serial),
			  PMLOGKS("Url",m_url.GetString().c_str()),
			  PMLOGKS("Exception",except.what()), "Unhandled exception occurred processing response %s",
			  MojoObjectJson(response).c_str());
	} catch (...) {
		LOG_AM_ERROR(MSGID_CALL_RESP_UNKNOWN_EXCEPTION, 2, PMLOGKFV("serial","%u",m_serial),
			PMLOGKS("Url",m_url.GetString().c_str()),
			"Unhandled exception of unknown type occurred processing response %s", MojoObjectJson(response).c_str());
	}
}

bool MojoCall::IsPermanentFailure(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
	if (err == MojErrNone)
		return false;

	if (!IsProtocolError(msg, response, err)) {
		return err;
	}

	MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
	if (!lunaMsg) {
		LOG_AM_ERROR(MSGID_NON_LUNA_BUS_MSG, 0, "IsPermanentFailure() : Message %s did not originate from the Luna Bus",
			  MojoObjectJson(response).c_str());
		throw std::runtime_error("Message did not originate from the "
			"Luna Bus");
	}

	const MojChar *method = lunaMsg->method();
	if (!method)
		return false;
	else if (!strcmp(method, LUNABUS_ERROR_PERMISSION_DENIED))
		return true;
	else if (!strcmp(method, LUNABUS_ERROR_SERVICE_NOT_EXIST))
		return true;
	else if (!strcmp(method, LUNABUS_ERROR_UNKNOWN_METHOD))
		return true;

	return false;
}

bool MojoCall::IsProtocolError(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
	if (err == MojErrNone)
		return false;

	MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
	if (!lunaMsg) {
		LOG_AM_ERROR(MSGID_NON_LUNA_BUS_MSG, 0, "IsProtocolError() : Message %s did not originate from the Luna Bus",
			  MojoObjectJson(response).c_str());
		throw std::runtime_error("Message did not originate from the Luna Bus");
	}

	const MojChar *category = lunaMsg->category();
	if (!category) {
		return false;
	} else {
		return (!strcmp(LUNABUS_ERROR_CATEGORY, lunaMsg->category()));
	}
}

void MojoCall::HandleResponse(MojObject& response, MojErr err)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Call %u] %s: Unhandled response \"%s\"",
		m_serial, m_url.GetString().c_str(), MojoObjectJson(response).c_str());
}

/* Thunk to old-style message, if not overridden */
void MojoCall::HandleResponse(MojServiceMessage *msg, MojObject& response,
	MojErr err)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	HandleResponse(response, err);
}

MojoCall::MojoCallMessageHandler::MojoCallMessageHandler(
	boost::shared_ptr<MojoCall> call, unsigned serial)
	: m_call(call)
	, m_responseSlot(this, &MojoCall::MojoCallMessageHandler::HandleResponse)
	, m_serial(serial)
{
}

MojoCall::MojoCallMessageHandler::~MojoCallMessageHandler()
{
}

MojServiceRequest::ExtendedReplySignal::Slot<MojoCall::MojoCallMessageHandler>&
MojoCall::MojoCallMessageHandler::GetResponseSlot()
{
	return m_responseSlot;
}

void MojoCall::MojoCallMessageHandler::Cancel()
{
	if (!m_call.expired()) {
		LOG_AM_DEBUG("[Call %u] [Handler %u] Cancelling",
			m_call.lock()->GetSerial(), m_serial);
	} else {
		LOG_AM_DEBUG("[Handler %u] Call expired, Cancelling",
			m_serial);
	}

	m_responseSlot.cancel();
}

MojErr MojoCall::MojoCallMessageHandler::HandleResponse(
	MojServiceMessage *msg, MojObject& response, MojErr err)
{
	if (m_call.expired()) {
		LOG_AM_DEBUG("[Handler %u] Response %s received for expired call", m_serial, MojoObjectJson(response).c_str());
		m_responseSlot.cancel();
		return MojErrInvalidArg;
	}

	m_call.lock()->HandleResponseWrapper(msg, response, err);

	return MojErrNone;
}

