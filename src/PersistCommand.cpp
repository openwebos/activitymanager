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
#include "Logging.h"

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
	LOG_TRACE("Entering function %s", __FUNCTION__);
	if (command == shared_from_this()) {
		LOG_WARNING(MSGID_APPEND_CMD_TOSELF,1,PMLOGKS("persist_command",GetString().c_str()),
			 "Attempt to append command directly to itself");
		throw std::runtime_error("Attempt to append Persist Command directly "
			"to itself");
	}

	boost::shared_ptr<PersistCommand> target;

	for (target = shared_from_this(); target->m_next;
		target = target->m_next) {
		if (target->m_next == shared_from_this()) {
			LOG_WARNING( MSGID_APPEND_CREATE_LOOP, 2, PMLOGKS("persist_command",command->GetString().c_str()),
                PMLOGKS("persist_command",target->GetString().c_str()),"Append Failed");
			throw std::runtime_error("Attempt to append Persist Command would "
				"create a loop");
		}
	}

	target->m_next = command;
}

void PersistCommand::Complete(bool success)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	/* All steps of Complete must execute, including launching the next
	 * command in the chain.  All exceptions must be handled locally. */
	try {
		m_completion->Complete(success);
	} catch (const std::exception& except) {
		LOG_WARNING(MSGID_CMD_COMPLETE_EXCEPTION,3,
			PMLOGKFV("activity","%llu",m_activity->GetId()),
		    PMLOGKS("persist_command",GetString().c_str()),
			PMLOGKS("exception",except.what()),"Unexpected exception while trying to complete");
	} catch (...) {
		LOG_WARNING(MSGID_CMD_COMPLETE_EXCEPTION,2,
			PMLOGKFV("activity","%llu",m_activity->GetId()),
		    PMLOGKS("persist_command",GetString().c_str()),"exception while trying to complete");

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
		LOG_WARNING( MSGID_CMD_UNHOOK_EXCEPTION,3,
			PMLOGKFV("activity","%llu",m_activity->GetId()),
            PMLOGKS("persist_command",GetString().c_str()),
			PMLOGKS("exception",except.what()),"Unexpected exception unhooking command" );
	} catch (...) {
		LOG_WARNING(MSGID_CMD_UNHOOK_EXCEPTION,2,
			PMLOGKFV("activity","%llu",m_activity->GetId()),
            PMLOGKS("persist_command",GetString().c_str()),"exception unhooking command");
	}

	if (next) {
		next->Persist();
	}
}

void PersistCommand::Validate(bool checkTokenValid) const
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	if (!m_activity->IsPersistTokenSet()) {
		LOG_ERROR(MSGID_PERSIST_TOKEN_NOT_SET,2,
			PMLOGKFV("activity","%llu",m_activity->GetId()),
 		    PMLOGKS("persist_command",GetString().c_str()),
            "Persist token for Activity is not set");

		throw std::runtime_error("Persist token for Activity is not set");
	}

	if (checkTokenValid) {
		boost::shared_ptr<PersistToken> pt = m_activity->GetPersistToken();
		if (!pt->IsValid()) {
			LOG_ERROR(MSGID_PERSIST_TOKEN_INVALID, 2,
				PMLOGKFV("activity","%llu",m_activity->GetId()),
 		        PMLOGKS("persist_command",GetString().c_str()),
			    "Persist token for Activity is set but not valid" );

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
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("[Activity %llu] [PersistCommand %s]: No-op command",
		m_activity->GetId(), GetString().c_str());
	Complete(true);
}

std::string NoopCommand::GetMethod() const
{
	return "Noop";
}

