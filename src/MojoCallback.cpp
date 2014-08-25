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

#include "MojoCallback.h"
#include "MojoTrigger.h"
#include "Activity.h"
#include "ActivityJson.h"
#include "Logging.h"

MojoCallback::MojoCallback(boost::shared_ptr<Activity> activity,
	MojService *service, const MojoURL& url, const MojObject& params)
	: Callback(activity)
	, m_service(service)
	, m_url(url)
	, m_params(params)
{
}

MojoCallback::~MojoCallback()
{
}

MojErr MojoCallback::Call()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);

	boost::shared_ptr<Activity> activity = m_activity.lock();

	/* Update the command sequence Serial number */
	SetSerial((unsigned)::random() % UINT_MAX);

	LOG_AM_DEBUG("[Activity %llu] Callback %s: Calling [Serial %u]",
		activity->GetId(), m_url.GetString().c_str(), GetSerial());

	MojErr err;

	/* Populate "$activity" */
	MojObject activityInfo;
	err = m_activity.lock()->ActivityInfoToJson(activityInfo);
	MojErrCheck(err);

	MojObject params = m_params;
	err = params.put(_T("$activity"), activityInfo);
	MojErrCheck(err);

	/* If an old MojoCall existed, it will be cancelled when its ref count
	 * drops... */
	m_call = boost::make_shared<MojoWeakPtrCall<MojoCallback> >(
		boost::dynamic_pointer_cast<MojoCallback, Callback>
			(shared_from_this()), &MojoCallback::HandleResponse,
			m_service, m_url, params);
	m_call->Call();

	return MojErrNone;
}

void MojoCallback::Cancel()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Callback %s: Cancelling",
		m_activity.lock()->GetId(), m_url.GetString().c_str());

	if (m_call) {
		m_call.reset();
	}
}

void MojoCallback::Failed(FailureType failure)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Callback %s: Failed%s",
		m_activity.lock()->GetId(), m_url.GetString().c_str(),
		(failure == TransientFailure) ? " (transient)" : "");

	m_call.reset();
	Callback::Failed(failure);
}

void MojoCallback::Succeeded()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Callback %s: Succeeded",
		m_activity.lock()->GetId(), m_url.GetString().c_str());

	m_call.reset();
	Callback::Succeeded();
}

void MojoCallback::HandleResponse(MojServiceMessage *msg,
	const MojObject& rep, MojErr err)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("[Activity %llu] Callback %s: Response %s",
		m_activity.lock()->GetId(), m_url.GetString().c_str(),
		MojoObjectJson(rep).c_str());

	/* All non-bus failures in a callback should be considered permanent
	 * failures. */
	if (err) {
		if (MojoCall::IsPermanentFailure(msg, rep, err)) {
			Failed(PermanentFailure);
		} else {
			Failed(TransientFailure);
		}
	} else {
		Succeeded();
	}
}

MojErr MojoCallback::ToJson(MojObject& rep, unsigned flags) const
{
	MojErr err;

	if (flags & ACTIVITY_JSON_CURRENT) {
		unsigned serial = GetSerial();
		if (serial != Callback::UnassignedSerial) {
			err = rep.putInt(_T("serial"), (MojInt64)serial);
			MojErrCheck(err);
		}
	} else {
		err = rep.putString(_T("method"), m_url.GetURL());
		MojErrCheck(err);

		if (m_params.type() != MojObject::TypeUndefined) {
			err = rep.put(_T("params"), m_params);
			MojErrCheck(err);
		}
	}

	return MojErrNone;
}

