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

#include "ControlGroup.h"
#include "ControlGroupManager.h"
#include "Logging.h"

#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

ControlGroup::ControlGroup(const std::string& name,
	boost::shared_ptr<ContainerManager> manager)
	: ResourceContainer(name, manager)
	, m_currentPriority(ActivityPriorityLow)
	, m_focused(false)
{
}

ControlGroup::~ControlGroup()
{
}

// Initialization is now a stub; it is no longer needed
// It is still useful for logging and may be used in the future.
void ControlGroup::Init()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Initializing control group [Container %s]",
		m_name.c_str());
}

void ControlGroup::UpdatePriority()
{
	// UNCOMMENT TO ENABLE CGROUPS HANDLING FROM ACTIVITYMANAGER

	/*
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);

	ActivityPriority_t priority = GetPriority();
	bool focused = IsFocused();

	if ((m_currentPriority != priority) || (m_focused != focused)) {
		MojLogDebug(s_log, _T("Updating priority for [Container %s] to "
			"[%s,%s] from [%s,%s]"), m_name.c_str(),
			ActivityPriorityNames[priority], focused ? "focused" : "unfocused",
			ActivityPriorityNames[m_currentPriority],
			m_focused ? "focused" : "unfocused");
		SetPriority(priority, focused);
	}
	*/
}

void ControlGroup::MapProcess(pid_t pid)
{
	// UNCOMMENT TO ENABLE CGROUPS HANDLING FROM ACTIVITYMANAGER
	/*
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	MojLogDebug(s_log, _T("Mapping [pid %d] into [Container %s]"),
		(int)pid, m_name.c_str());
	
	// Store all pids for containter
	m_processIds.push_back(pid);

	// Explicitly set the priority as the new pid is unmapped atm
	SetPriority(GetPriority(), IsFocused());
	*/
}

void ControlGroup::Enable()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	UpdatePriority();
}

void ControlGroup::Disable()
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	UpdatePriority();
}

MojErr ControlGroup::ToJson(MojObject& rep) const
{
	std::string path = GetRoot() + "/" + m_name;

	MojErr err = rep.putString(_T("path"), path.c_str());
	MojErrCheck(err);

	return ResourceContainer::ToJson(rep);
}

/* places all processes of current container into either the focused  *
 * cgroup or to the sub-cgroup of the given priority                  */
void ControlGroup::SetPriority(ActivityPriority_t priority, bool focused)
{
	bool success = true;
	char buf[8];
	size_t length;
	std::list<int>::iterator pid, nextPid;

	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Setting priority of [Container %s] to [%s,%s]",
		m_name.c_str(), ActivityPriorityNames[priority],
		focused ? "focused" : "unfocused");

	 const char* shares[] = {
		"none", 	/* ActivityPriorityNone */
		"lowest",	/* ActivityPriorityLowest */
		"low",  	/* ActivityPriorityLow */
		"normal",	/* ActivityPriorityNormal */
		"high", 	/* ActivityPriorityHigh */
		"highest"	/* ActivityPriorityHighest */
	};

        std::string controlFile;
        if (focused) {
                controlFile = GetRoot() + "/focused/tasks";
        } else {
                controlFile = GetRoot() + "/unfocused/" + shares[priority] + "/tasks";
        }

	pid = m_processIds.begin();

	while(pid!= m_processIds.end()){
		nextPid = pid;
		nextPid++;

                length = snprintf(buf, 8, "%d", (int)*pid);
                success = success && WriteControlFile(controlFile, buf, length, pid);

		// Fix for roadrunner
		if(pid == m_processIds.begin() && !success && !focused){
			controlFile = GetRoot() + "/unfocused/tasks";
			success = success || WriteControlFile(controlFile, buf, length, pid);
		}

		pid = nextPid;
    }

	if (success) {
		m_currentPriority = priority;
		m_focused = focused;
	} else {
		LOG_AM_WARNING(MSGID_CONTAINER_PRIORITY_CHANGE_FAIL, 3, PMLOGKS("container",m_name.c_str()),
			  PMLOGKS("Priority",ActivityPriorityNames[priority]),
			  PMLOGKS("focus",focused ? "focused" : "unfocused"), "");
	}
}

const std::string& ControlGroup::GetRoot() const
{
	return boost::dynamic_pointer_cast<ControlGroupManager, ContainerManager>
		(m_manager.lock())->GetRoot();
}

bool ControlGroup::WriteControlFile(const std::string& controlFile,
	const char *buf, size_t count, std::list<int>::iterator current)
{
	int fd = open(controlFile.c_str(), O_WRONLY);
	if (fd < 0) {
		LOG_AM_ERROR(MSGID_CONTROL_FILE_OPEN_FAIL, 2, PMLOGKS("control_file",controlFile.c_str()),
			  PMLOGKS("Reason",strerror(errno)), "");
		return false;
	}

	ssize_t ret = write(fd, buf, count);
	if (ret < 0) {

		// If the process does not exist, cgroups will return ESRCH (no such process)
		if(errno == ESRCH){
			LOG_AM_WARNING(MSGID_INEXISTENT_PROCESS_WRITTEN, 2, PMLOGKS("Process",buf),
				    PMLOGKS("control_file",controlFile.c_str()), "Process to be written to file doesn't exist");
			close(fd);
			m_processIds.erase(current);
			return true;
		}
		LOG_AM_ERROR(MSGID_WRITE_TO_CONTROL_FILE_FAIL, 2, PMLOGKS("control_file",controlFile.c_str()),
			  PMLOGKS("Reason",strerror(errno)), "");
		close(fd);
		return false;
	} else if ((size_t)ret != count) {
		LOG_AM_ERROR(MSGID_SHORT_WRITE_UPDATE, 1, PMLOGKS("control_file",controlFile.c_str()), "");
		close(fd);
		return false;
	}

	close(fd);
	return true;
}

NullControlGroup::NullControlGroup(const std::string& name,
	boost::shared_ptr<ContainerManager> manager)
	: ControlGroup(name, manager)
{
}

NullControlGroup::~NullControlGroup()
{
}

bool NullControlGroup::WriteControlFile(const std::string& controlFile,
	const char *buf, size_t count, std::list<int>::iterator current)
{
	return true;
}

