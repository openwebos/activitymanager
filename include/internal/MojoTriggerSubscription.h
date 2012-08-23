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

#ifndef __ACTIVITYMANAGER_MOJOTRIGGERSUBCRIPTION_H__
#define __ACTIVITYMANAGER_MOJOTRIGGERSUBSCRITION_H__

#include "Base.h"
#include "MojoURL.h"

class MojoTrigger;
class MojoExclusiveTrigger;
class MojoCall;

class MojoTriggerSubscription :
	public boost::enable_shared_from_this<MojoTriggerSubscription>
{
public:
	MojoTriggerSubscription(boost::shared_ptr<MojoTrigger> trigger,
		MojService *service, const MojoURL& url,
		const MojObject& params);
	virtual ~MojoTriggerSubscription();

	const MojoURL& GetURL() const;

	virtual void Subscribe();
	void Unsubscribe();

	bool IsSubscribed() const;

	MojErr ToJson(MojObject& rep, unsigned flags) const;

protected:
	void ProcessResponse(MojServiceMessage *msg, const MojObject& response,
		MojErr err);

	boost::weak_ptr<MojoTrigger>	m_trigger;

	MojService	*m_service;
	MojoURL		m_url;
	MojObject	m_params;

	boost::shared_ptr<MojoCall>	m_call;

	static MojLogger	s_log;
};

class MojoExclusiveTriggerSubscription : public MojoTriggerSubscription
{
public:
	MojoExclusiveTriggerSubscription(boost::shared_ptr<MojoExclusiveTrigger>
		trigger, MojService *service, const MojoURL& url,
		const MojObject& params);
	virtual ~MojoExclusiveTriggerSubscription();

	virtual void Subscribe();
};

#endif /* __ACTIVITYMANAGER_MOJOTRIGGERSUBSCRIPTION_H__ */
