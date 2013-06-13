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

#ifndef __ACTIVITYMANAGER_MOJODBPROXY_H__
#define __ACTIVITYMANAGER_MOJODBPROXY_H__

#include "PersistProxy.h"

#include <list>

class Activity;
class ActivityManager;
class ActivityManagerApp;
class Completion;
class MojoJsonConverter;
class MojoCall;
class MojoDBPersistToken;

class MojoDBProxy : public PersistProxy
{
public:
	MojoDBProxy(ActivityManagerApp *app, MojService *service,
		boost::shared_ptr<ActivityManager> am,
		boost::shared_ptr<MojoJsonConverter> json);
	virtual ~MojoDBProxy();

	virtual boost::shared_ptr<PersistCommand> PrepareStoreCommand(
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion);
	virtual boost::shared_ptr<PersistCommand> PrepareDeleteCommand(
		boost::shared_ptr<Activity> activity,
		boost::shared_ptr<Completion> completion);

	virtual boost::shared_ptr<PersistToken> CreateToken();

	virtual void LoadActivities();

	static const char *ActivityKind;

protected:
	void ActivityLoadResults(MojServiceMessage *msg, const MojObject& response,
		MojErr err);
	void ActivityPurgeComplete(MojServiceMessage *msg,
		const MojObject& response, MojErr err);
	void ActivityConfiguratorComplete(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	static const int PurgeBatchSize;

	void PreparePurgeCall();
	void PrepareConfiguratorCall();

	void PopulatePurgeIds(MojObject& ids);

	ActivityManagerApp	*m_app;
	MojService			*m_service;

	boost::shared_ptr<ActivityManager>		m_am;
	boost::shared_ptr<MojoJsonConverter>	m_json;

	/* Callout for loading content from MojoDB in through the Proxy,
	 * and later deleting bad data. */
	boost::shared_ptr<MojoCall>	m_call;

	/* Track old Activities that should be purged */
	typedef std::list<boost::shared_ptr<MojoDBPersistToken> > TokenQueue;
	TokenQueue	m_oldTokens;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_MOJODBPROXY_H__ */
