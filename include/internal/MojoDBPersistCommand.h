/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef __ACTIVITYMANAGER_MOJODBPERSISTCOMMAND_H__
#define __ACTIVITYMANAGER_MOJODBPERSISTCOMMAND_H__

#include "MojoPersistCommand.h"

class Activity;

class MojoDBStoreCommand : public MojoPersistCommand
{
public:
	MojoDBStoreCommand(MojService *service,
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion);
	virtual ~MojoDBStoreCommand();

protected:
	virtual std::string GetMethod() const;

	virtual void UpdateParams(MojObject& params);
	virtual void PersistResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
};

class MojoDBDeleteCommand : public MojoPersistCommand
{
public:
	MojoDBDeleteCommand(MojService *service,
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion);
	virtual ~MojoDBDeleteCommand();

protected:
	virtual std::string GetMethod() const;

	virtual void UpdateParams(MojObject& params);
	virtual void PersistResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
};

#endif /* __ACTIVITYMANAGER_MOJODBPERSISTCOMMAND_H__ */
