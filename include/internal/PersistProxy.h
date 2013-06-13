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

#ifndef __ACTIVITYMANAGER_PERSISTPROXY_H__
#define __ACTIVITYMANAGER_PERSISTPROXY_H__

#include "Base.h"
#include "PersistCommand.h"

class Activity;
class Completion;
class PersistToken;

class PersistProxy : public boost::enable_shared_from_this<PersistProxy>
{
public:
	PersistProxy();
	virtual ~PersistProxy();

	typedef enum CommmandType_e { StoreCommandType, DeleteCommandType,
		NoopCommandType } CommandType;

	virtual boost::shared_ptr<PersistCommand> PrepareCommand(
		CommandType type, boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion);

	virtual boost::shared_ptr<PersistCommand> PrepareStoreCommand(
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion) = 0;
	virtual boost::shared_ptr<PersistCommand> PrepareDeleteCommand(
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion) = 0;
	virtual boost::shared_ptr<PersistCommand> PrepareNoopCommand(
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion);

	virtual boost::shared_ptr<PersistToken> CreateToken() = 0;

	virtual void LoadActivities() = 0;

protected:
	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_PERSISTPROXY_H__ */
