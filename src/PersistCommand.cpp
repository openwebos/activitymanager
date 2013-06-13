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

#include "PersistCommand.h"
#include "PersistToken.h"
#include "Completion.h"
#include "Activity.h"

#include <stdexcept>

MojLogger PersistCommand::s_log(_T("activitymanager.persistcommand"));

PersistCommand::PersistCommand(boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
	: m_activity(activity)
	, m_completion(completion)
{
}

PersistCommand::~PersistCommand()
{
}

boost::shared_ptr<Activity> PersistCommand::GetActivity()
{
	return m_activity;
}

std::string PersistCommand::GetString() const
{
	if (m_activity->IsPersistTokenSet()) {
		return (GetMethod() + " " + m_activity->GetPersistToken()->GetString());
	} else {
		return GetMethod() + "(no token)";
	}
}

/* Append the new command to the end of the command chain that this
 * command is a member of. */
void PersistCommand::Append(boost::shared_ptr<PersistCommand> command)
{
	MojLogTrace(s_log);
	if (command == shared_from_this()) {
		MojLogError(s_log, _T("[PersistCommand %s]: Attempt to append command "
			"directly to itself"), GetString().c_str());
		throw std::runtime_error("Attempt to append Persist Command directly "
			"to itself");
	}

	boost::shared_ptr<PersistCommand> target;

	for (target = shared_from_this(); target->m_next;
		target = target->m_next) {
		if (target->m_next == shared_from_this()) {
			MojLogError(s_log, _T("Append failed: [PersistCommand %s] already "
				"referenced from [PersistCommand %s]"),
				command->GetString().c_str(), target->GetString().c_str());
			throw std::runtime_error("Attempt to append Persist Command would "
				"create a loop");
		}
	}

	target->m_next = command;
}

void PersistCommand::Complete(bool success)
{
	MojLogTrace(s_log);

	/* All steps of Complete must execute, including launching the next
	 * command in the chain.  All exceptions must be handled locally. */
	try {
		m_completion->Complete(success);
	} catch (const std::exception& except) {
		MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
			"Unexpected exception %s while trying to complete"),
			m_activity->GetId(), GetString().c_str(), except.what());
	} catch (...) {
		MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: Unknown "
			"exception while trying to complete"), m_activity->GetId(),
			GetString().c_str());
	}

	/* Tricky order here.  Unhooking this command will probably destroy it,
	 * so we have to pick up a local reference to the next command in the
	 * chain because we won't be able to access the object property.
	 *
	 * The whole chain shouldn't collapse, though, as the next executing
	 * command should be referenced by the same Activity or some other one.
	 * It's just the reference that's bad... but since something like:
	 * Complete "foo"/Create(and replace) "foo"/Cancel old "foo".  If the
	 * Complete is still in process, the Cancel will chain (and ultimately
	 * will succeed as the Activity is already cancelled).
	 */
	boost::shared_ptr<PersistCommand> next = m_next;

	try {
		m_activity->UnhookPersistCommand(shared_from_this());
	} catch (const std::exception& except) {
		MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
			"Unexpected exception \"%s\" unhooking command"),
			m_activity->GetId(), GetString().c_str(), except.what());
	} catch (...) {
		MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: Unknown "
			"exception unhooking command"), m_activity->GetId(),
			GetString().c_str());
	}

	if (next) {
		next->Persist();
	}
}

void PersistCommand::Validate(bool checkTokenValid) const
{
	MojLogTrace(s_log);

	if (!m_activity->IsPersistTokenSet()) {
		MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: Persist "
			"Token is not set"), m_activity->GetId(), GetString().c_str());
		throw std::runtime_error("Persist token for Activity is not set");
	}

	if (checkTokenValid) {
		boost::shared_ptr<PersistToken> pt = m_activity->GetPersistToken();
		if (!pt->IsValid()) {
			MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
				"Persist Token is not valid"), m_activity->GetId(),
				GetString().c_str());
			throw std::runtime_error("Persist token for Activity is set "
				"but not valid");
		}
	}
}

NoopCommand::NoopCommand(boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
	: PersistCommand(activity, completion)
{
}

NoopCommand::~NoopCommand()
{
}

void NoopCommand::Persist()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] [PersistCommand %s]: No-op command"),
		m_activity->GetId(), GetString().c_str());
	Complete(true);
}

std::string NoopCommand::GetMethod() const
{
	return "Noop";
}

