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

#include "PersistProxy.h"
#include "PersistCommand.h"
#include "Activity.h"
#include "Logging.h"

#include <stdexcept>

MojLogger PersistProxy::s_log(_T("activitymanager.persistproxy"));

PersistProxy::PersistProxy()
{
}

PersistProxy::~PersistProxy()
{
}

boost::shared_ptr<PersistCommand>
PersistProxy::PrepareCommand(CommandType type,
	boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
{
	switch (type) {
	case StoreCommandType:
		return PrepareStoreCommand(activity, completion);
	case DeleteCommandType:
		return PrepareDeleteCommand(activity, completion);
	case NoopCommandType:
		return PrepareNoopCommand(activity, completion);
	}

	throw std::runtime_error("Attempt to create command of unknown type");
}

boost::shared_ptr<PersistCommand>
PersistProxy::PrepareNoopCommand(boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
{
	LOG_AM_TRACE("Entering function %s", __FUNCTION__);
	LOG_AM_DEBUG("Preparing noop command for [Activity %llu]",
		activity->GetId());

	return boost::make_shared<NoopCommand>(activity, completion);
}

