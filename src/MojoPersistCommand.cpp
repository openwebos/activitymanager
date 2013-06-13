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
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] [PersistCommand %s]: Issuing"),
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
		MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
			"Unexpected exception \"%s\" while attempting to persist"),
			m_activity->GetId(), GetString().c_str(), except.what());
		Complete(false);
	} catch (...) {
		MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
			"Unknown exception while attempting to persist"),
			m_activity->GetId(), GetString().c_str());
		Complete(false);
	}
}

void MojoPersistCommand::PersistResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: Failed "
				"with error \"%s\", code %d"), m_activity->GetId(),
				GetString().c_str(),
				MojoObjectString(response, _T("errorText")).c_str(), (int)err);
			Complete(false);
		} else {
			MojLogWarning(s_log, _T("[Activity %llu] [PersistCommand %s]: "
				"Failed with transient error, retrying: %s"),
				m_activity->GetId(), GetString().c_str(),
				MojoObjectJson(response).c_str());
			m_call->Call();
		}
	} else {
		MojLogInfo(s_log, _T("[Activity %llu] [PersistCommand %s]: Succeeded"),
			m_activity->GetId(), GetString().c_str());
		Complete(true);
	}
}

