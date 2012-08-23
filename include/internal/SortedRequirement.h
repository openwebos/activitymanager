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

#ifndef __ACTIVITYMANAGER_SORTEDREQUIREMENT_H__
#define __ACTIVITYMANAGER_SORTEDREQUIREMENT_H__

#include "Requirement.h"
#include <boost/intrusive/set.hpp>

template <class T>
struct FriendMaker {
	typedef T Type;
};

template <typename K, class M>
class SortedRequirement : public Requirement
{
public:
	SortedRequirement(boost::shared_ptr<Activity> activity,
		K key, bool met = false)
		: Requirement(activity, met), m_key(key) {};
	virtual ~SortedRequirement() {};

	bool operator<(const SortedRequirement& rhs) const
		{ return (m_key < rhs.m_key); };

protected:
	friend class FriendMaker<M>::Type;

	typedef boost::intrusive::set_member_hook<
		boost::intrusive::link_mode<
			boost::intrusive::auto_unlink> > TableItem;

	K 			m_key;
	TableItem	m_item;

	typedef boost::intrusive::member_hook<SortedRequirement,
		TableItem, &SortedRequirement::m_item> TableOption;
	typedef boost::intrusive::set<SortedRequirement, TableOption,
		boost::intrusive::constant_time_size<false> > Table;
	typedef boost::intrusive::multiset<SortedRequirement, TableOption,
		boost::intrusive::constant_time_size<false> > MultiTable;

	struct KeyComp {
		bool operator()(const K& key, const SortedRequirement& req) const
			{ return (key < req.m_key); };
		bool operator()(const SortedRequirement& req, const K& key) const
			{ return (req.m_key < key); };
	};
};

#endif /* __ACTIVITYMANAGER_SORTEDREQUIREMENT_H__ */
