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

#ifndef __ACTIVITYMANAGER_SUBSCRIBER_H__
#define __ACTIVITYMANAGER_SUBSCRIBER_H__

#include "Base.h"
#include "BusId.h"

#include <string>
#include <boost/intrusive/set.hpp>

class Activity;

class Subscriber {
public:
	Subscriber(const std::string& id, BusIdType type);
	Subscriber(const char *id, BusIdType type);
	virtual ~Subscriber();

	BusIdType GetType() const;
	const BusId& GetId() const;

	std::string GetString() const;

	MojErr ToJson(MojObject& rep) const;

	bool operator<(const Subscriber& rhs) const;
	bool operator!=(const Subscriber& rhs) const;
	bool operator==(const Subscriber& rhs) const;

	bool operator!=(const BusId& rhs) const;
	bool operator==(const BusId& rhs) const;

	bool operator!=(const std::string& rhs) const;
	bool operator==(const std::string& rhs) const;

private:
	Subscriber(const Subscriber& subscriber);
	Subscriber& operator=(const Subscriber& subscriber);

protected:
	Subscriber();

	friend class Activity;

	typedef boost::intrusive::set_member_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink> > SetItem;
	SetItem	m_item;

	friend class BusId;

	BusId	m_id;
};

#endif /* __ACTIVITYMANAGER_SUBSCRIBER_H__ */
