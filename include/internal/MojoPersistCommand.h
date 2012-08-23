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

#ifndef __ACTIVITYMANAGER_MOJOPERSISTCOMMAND_H__
#define __ACTIVITYMANAGER_MOJOPERSISTCOMMAND_H__

#include "PersistCommand.h"
#include "MojoURL.h"

#include <core/MojService.h>

class MojoCall;
class Completion;
class Activity;

class MojoPersistCommand : public PersistCommand
{
public:
	MojoPersistCommand(MojService *service, const MojoURL& method,
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion);
	MojoPersistCommand(MojService *service, const MojoURL& method,
		const MojObject& params, boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion);
	virtual ~MojoPersistCommand();

	virtual void Persist();

protected:
	std::string GetIdString() const;

	virtual void UpdateParams(MojObject& params) = 0;
	virtual void PersistResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	MojService	*m_service;
	MojoURL		m_method;
	MojObject	m_params;

	boost::shared_ptr<MojoCall>		m_call;
};


#endif /* __ACTIVITYMANAGER_MOJOPERSISTCOMMAND_H__ */
