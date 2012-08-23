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

#ifndef __ACTIVITYMANAGER_BUSID_H__
#define __ACTIVITYMANAGER_BUSID_H__

#include "Base.h"
#include <string>

enum BusIdType {
	BusNull,
	BusApp,
	BusService,
	BusAnon
};

class Subscriber;

class BusId {
public:
	BusId();
	BusId(const std::string& id, BusIdType type);
	BusId(const char *id, BusIdType type);
	BusId(const Subscriber& subscriber);

	BusIdType GetType() const;
	std::string GetId() const;

	std::string GetString() const;
	static std::string GetString(const std::string& id, BusIdType type);
	static std::string GetString(const char *id, BusIdType type);

	MojErr ToJson(MojObject& rep) const;

	BusId& operator=(const Subscriber& rhs);

	bool operator<(const BusId& rhs) const;
	bool operator!=(const BusId& rhs) const;
	bool operator==(const BusId& rhs) const;

	bool operator!=(const Subscriber& rhs) const;
	bool operator==(const Subscriber& rhs) const;

	bool operator!=(const std::string& rhs) const;
	bool operator==(const std::string& rhs) const;

protected:
	std::string	m_id;
	BusIdType	m_type;
};

#endif /* __ACTIVITYMANAGER_BUSID_H__ */
