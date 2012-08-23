// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "MojoTriggerSubscription.h"
#include "MojoTrigger.h"
#include "MojoCall.h"

MojLogger MojoTriggerSubscription::s_log(_T("activitymanager.triggersubscription"));

MojoTriggerSubscription::MojoTriggerSubscription(
	boost::shared_ptr<MojoTrigger> trigger, MojService *service,
	const MojoURL& url, const MojObject& params)
	: m_trigger(trigger)
	, m_service(service)
	, m_url(url)
	, m_params(params)
{
}

MojoTriggerSubscription::~MojoTriggerSubscription()
{
}

const MojoURL& MojoTriggerSubscription::GetURL() const
{
	return m_url;
}

void MojoTriggerSubscription::Subscribe()
{
	if (!m_call) {
		m_call = boost::make_shared<MojoWeakPtrCall<MojoTriggerSubscription> >
			(shared_from_this(), &MojoTriggerSubscription::ProcessResponse,
			m_service, m_url, m_params, MojoCall::Unlimited);
		m_call->Call();
	}
}

void MojoTriggerSubscription::Unsubscribe()
{
	if (m_call) {
		m_call.reset();
	}
}

bool MojoTriggerSubscription::IsSubscribed() const
{
	return m_call;
}

void MojoTriggerSubscription::ProcessResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	if (err != MojErrNone) {
		if (!MojoCall::IsPermanentFailure(msg, response, err)) {
			m_call->Call();
			return;
		}
	}

	m_trigger.lock()->ProcessResponse(response, err);
}

MojErr MojoTriggerSubscription::ToJson(MojObject& rep, unsigned flags) const
{
	MojErr err;

	err = rep.putString(_T("method"), m_url.GetURL());
	MojErrCheck(err);

	if (m_params.type() != MojObject::TypeUndefined) {
		err = rep.put(_T("params"), m_params);
		MojErrCheck(err);
	}

	return MojErrNone;
}

MojoExclusiveTriggerSubscription::MojoExclusiveTriggerSubscription(
	boost::shared_ptr<MojoExclusiveTrigger> trigger, MojService *service,
	const MojoURL& url, const MojObject& params)
	: MojoTriggerSubscription(trigger, service, url, params)
{
}

MojoExclusiveTriggerSubscription::~MojoExclusiveTriggerSubscription()
{
}

void MojoExclusiveTriggerSubscription::Subscribe()
{
	if (!m_call) {
		m_call = boost::make_shared<MojoWeakPtrCall<MojoTriggerSubscription> >
			(shared_from_this(),
			&MojoExclusiveTriggerSubscription::ProcessResponse,
			m_service, m_url, m_params, MojoCall::Unlimited);
		m_call->Call(boost::dynamic_pointer_cast<MojoExclusiveTrigger,
			MojoTrigger>(m_trigger.lock())->GetActivity());
	}
}

