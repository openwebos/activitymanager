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

#ifndef __ACTIVITYMANAGER_MOJOCALLBACK_H__
#define __ACTIVITYMANAGER_MOJOCALLBACK_H__

#include "Callback.h"
#include "MojoCall.h"

#include <core/MojService.h>

class MojoCallback : public Callback
{
public:
	MojoCallback(boost::shared_ptr<Activity> activity, MojService *service,
		const MojoURL& url, const MojObject& params);
	virtual ~MojoCallback();

	virtual MojErr Call();
	virtual void Cancel();

	virtual void Failed(FailureType failure);
	virtual void Succeeded();

	virtual MojErr ToJson(MojObject& rep, unsigned flags) const;

protected:
	virtual void HandleResponse(MojServiceMessage *msg,
		const MojObject& response, MojErr err);

	MojService	*m_service;
	MojoURL		m_url;
	MojObject	m_params;

	boost::shared_ptr<MojoWeakPtrCall<MojoCallback> >	m_call;
};

#endif /* __ACTIVITYMANAGER_MOJOCALLBACK_H__ */
