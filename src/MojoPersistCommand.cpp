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

#include "MojoPersistCommand.h"
#include "MojoCall.h"
#include "Completion.h"
#include "Activity.h"
#include "MojoObjectWrapper.h"
#include "Logging.h"

#include <stdexcept>

MojoPersistCommand::MojoPersistCommand(MojService *service,
	const MojoURL& method, boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
	: PersistCommand(activity, completion)
	, m_service(service)
	, m_method(method)
{
}

MojoPersistCommand::MojoPersistCommand(MojService *service,
	const MojoURL& method, const MojObject& params,
	boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
	: PersistCommand(activity, completion)
	, m_service(service)
	, m_method(method)
	, m_params(params)
{
}

MojoPersistCommand::~MojoPersistCommand()
{
}

void MojoPersistCommand::Persist()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] [PersistCommand %s]: Issuing",
		m_activity->GetId(), GetString().c_str());

	/* Perform update of Parameters, if desired - modify copy, not original,
	 * to ensure old data isn't retained between calls */
	try {
		MojObject params = m_params;
		UpdateParams(params);

		m_call = boost::make_shared<MojoWeakPtrCall<MojoPersistCommand> >
			(boost::dynamic_pointer_cast<MojoPersistCommand, PersistCommand>
				(shared_from_this()),
			&MojoPersistCommand::PersistResponse, m_service, m_method, params);
		m_call->Call();
	} catch (const std::exception& except) {
		LOG_ERROR(MSGID_PERSIST_ATMPT_UNEXPECTD_EXCPTN, 2, PMLOGKFV("activity","%llu",m_activity->GetId()),
			  PMLOGKS("Persist_command",GetString().c_str()),PMLOGKS("Exception",except.what()),
			  "Unexpected exception while attempting to persist");
		Complete(false);
	} catch (...) {
		LOG_ERROR(MSGID_PERSIST_ATMPT_UNKNWN_EXCPTN, 2, PMLOGKFV("activity","%llu",m_activity->GetId()),
			  PMLOGKS("Persist_command",GetString().c_str()), "Unknown exception while attempting to persist");
		Complete(false);
	}
}

void MojoPersistCommand::PersistResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			LOG_WARNING(MSGID_PERSIST_CMD_RESP_FAIL, 4, PMLOGKFV("activity","%llu",m_activity->GetId()),
				  PMLOGKS("persist_command",GetString().c_str()),
				  PMLOGKS("Errtext",MojoObjectString(response, _T("errorText")).c_str()),
				  PMLOGKFV("Errcode","%d",(int)err), "");
			Complete(false);
		} else {
			LOG_WARNING(MSGID_PERSIST_CMD_TRANSIENT_ERR, 2, PMLOGKFV("activity","%llu",m_activity->GetId()),
				    PMLOGKS("persist_command",GetString().c_str()), "Failed with transient error, retrying: %s",
				    MojoObjectJson(response).c_str());
			m_call->Call();
		}
	} else {
		LOG_DEBUG("[Activity %llu] [PersistCommand %s]: Succeeded",
			m_activity->GetId(), GetString().c_str());
		Complete(true);
	}
}

