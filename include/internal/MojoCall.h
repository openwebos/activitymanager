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

#ifndef __ACTIVITYMANAGER_MOJOCALL_H__
#define __ACTIVITYMANAGER_MOJOCALL_H__

#include "Base.h"
#include "MojoURL.h"

#include <core/MojService.h>
#include <core/MojObject.h>
#include <core/MojServiceRequest.h>

class Activity;

class MojoCall : public boost::enable_shared_from_this<MojoCall>
{
public:
	MojoCall(MojService *service, const MojoURL& url, const MojObject& params,
		MojUInt32 replies = 1);
	virtual ~MojoCall();

	static const MojUInt32 Unlimited;

	MojErr Call();
	MojErr Call(boost::shared_ptr<Activity> activity);
	MojErr Call(bool usePublicBus, const char *proxyRequester);

	void Cancel();

	unsigned GetSerial() const;

	static bool IsPermanentFailure(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
	static bool IsProtocolError(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

protected:

	void HandleResponseWrapper(MojServiceMessage *msg, MojObject& response, MojErr err);

	virtual void HandleResponse(MojObject& response, MojErr err);
	virtual void HandleResponse(MojServiceMessage *msg, MojObject& response, MojErr err);

	class MojoCallMessageHandler : public MojSignalHandler
	{
	public:
		MojoCallMessageHandler(boost::shared_ptr<MojoCall> call,
			unsigned serial);
		~MojoCallMessageHandler();

		MojServiceRequest::ExtendedReplySignal::Slot<MojoCallMessageHandler>& GetResponseSlot();
		void Cancel();

	protected:
		MojErr HandleResponse(MojServiceMessage *msg, MojObject& response, MojErr err);

		boost::weak_ptr<MojoCall> m_call;
		MojServiceRequest::ExtendedReplySignal::Slot<MojoCallMessageHandler> m_responseSlot;
		unsigned m_serial;
	};

	friend class MojoCallMessageHandler;

	MojService	*m_service;
	MojoURL		m_url;
	MojObject	m_params;

	MojUInt32	m_replies;

	MojRefCountedPtr<MojoCall::MojoCallMessageHandler>	m_handler;

	unsigned	m_serial;
	unsigned	m_subSerial;

	static unsigned		s_serial;
	static MojLogger	s_log;
};

template<class T>
class MojoPtrCall : public MojoCall
{
public:
	typedef void (T::* CallbackType)(const MojObject& response, MojErr err);

	MojoPtrCall(boost::shared_ptr<T> ptr, CallbackType callback,
		MojService *service, const MojoURL& url, const MojObject& params,
		MojUInt32 replies = 1)
		: MojoCall(service, url, params, replies)
		, m_ptr(ptr)
		, m_callback(callback)
	{
	}

	virtual ~MojoPtrCall() {}

protected:
	virtual void HandleResponse(MojObject& response, MojErr err)
	{
		((*m_ptr.get()).*m_callback)(response, err);
	}

	boost::shared_ptr<T>	m_ptr;
	CallbackType			m_callback;
};

template<class T>
class MojoPtrCall2 : public MojoCall
{
public:
	typedef void (T::* CallbackType)(boost::shared_ptr<MojoCall> call,
		const MojObject& response, MojErr err);

	MojoPtrCall2(boost::shared_ptr<T> ptr, CallbackType callback,
		MojService *service, const MojoURL& url, const MojObject& params,
		MojUInt32 replies = 1)
		: MojoCall(service, url, params, replies)
		, m_ptr(ptr)
		, m_callback(callback)
	{
	}

	virtual ~MojoPtrCall2() {}

protected:
	virtual void HandleResponse(MojObject& response, MojErr err)
	{
		((*m_ptr.get()).*m_callback)(shared_from_this(), response, err);
	}

	boost::shared_ptr<T>	m_ptr;
	CallbackType			m_callback;
};

template<class T>
class MojoWeakPtrCall : public MojoCall
{
public:
	typedef void (T::* CallbackType)(MojServiceMessage *msg, const MojObject& response, MojErr err);

	MojoWeakPtrCall(boost::shared_ptr<T> ptr, CallbackType callback,
		MojService *service, const MojoURL& url, const MojObject& params,
		MojUInt32 replies = 1)
		: MojoCall(service, url, params, replies)
		, m_ptr(ptr)
		, m_callback(callback)
	{
	}

	virtual ~MojoWeakPtrCall() {}

protected:
	virtual void HandleResponse(MojServiceMessage *msg, MojObject& response,
		MojErr err)
	{
		if (!m_ptr.expired()) {
			((*m_ptr.lock().get()).*m_callback)(msg, response, err);
		}
	}

	boost::weak_ptr<T>	m_ptr;
	CallbackType		m_callback;
};

template<class T>
class MojoWeakPtrCall2 : public MojoCall
{
public:
	typedef void (T::* CallbackType)(boost::shared_ptr<MojoCall> call,
		const MojObject& response, MojErr err);

	MojoWeakPtrCall2(boost::shared_ptr<T> ptr, CallbackType callback,
		MojService *service, const MojoURL& url, const MojObject& params,
		MojUInt32 replies = 1)
		: MojoCall(service, url, params, replies)
		, m_ptr(ptr)
		, m_callback(callback)
	{
	}

	virtual ~MojoWeakPtrCall2() {}

protected:
	virtual void HandleResponse(MojObject& response, MojErr err)
	{
		if (!m_ptr.expired()) {
			((*m_ptr.lock().get()).*m_callback)(shared_from_this(),
				response, err);
		}
	}

	boost::weak_ptr<T>	m_ptr;
	CallbackType		m_callback;
};

#endif /* __ACTIVITYMANAGER_MOJOCALLH__ */
