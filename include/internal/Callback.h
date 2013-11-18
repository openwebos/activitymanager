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

#ifndef __ACTIVITYMANAGER_CALLBACK_H__
#define __ACTIVITYMANAGER_CALLBACK_H__

#include "Base.h"

#include <climits>

class Activity;

class Callback : public boost::enable_shared_from_this<Callback>
{
public:
	Callback(boost::shared_ptr<Activity> activity);
	virtual ~Callback();

	virtual MojErr Call() = 0;
	virtual void Cancel() = 0;

	enum FailureType { PermanentFailure, TransientFailure };

	virtual void Failed(FailureType failure);
	virtual void Succeeded();

	unsigned GetSerial() const;
	void SetSerial(unsigned serial);

	virtual MojErr ToJson(MojObject& rep, unsigned flags) const = 0;

	static const unsigned UnassignedSerial = UINT_MAX;

protected:
	bool	m_complete;

	unsigned	m_serial;

	boost::weak_ptr<Activity> m_activity;

	static MojLogger	s_log;
};

#endif /* __ACTIVITYMANAGER_CALLBACK_H__ */
