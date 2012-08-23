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

#ifndef __ACTIVITYMANAGER_MOJOWHEREMATCHER_H__
#define __ACTIVITYMANAGER_MOJOWHEREMATCHER_H__

#include "Base.h"
#include "MojoMatcher.h"

/*
 * "where" : [{
 *     "prop" : <property name> | [{ <property name>, ... }]
 *     "op" : "<" | "<=" | "=" | ">=" | ">" | "!="
 *     "val" : <comparison value>
 * }]
 */
class MojoNewWhereMatcher : public MojoMatcher
{
public:
	MojoNewWhereMatcher(const MojObject& where);
	virtual ~MojoNewWhereMatcher();

	virtual bool Match(const MojObject& response);

	virtual MojErr ToJson(MojObject& rep, unsigned long flags) const;

protected:
	void ValidateKey(const MojObject& key) const;
	void ValidateOp(const MojObject& op, const MojObject& val) const;
	void ValidateClause(const MojObject& clause) const;
	void ValidateClauses(const MojObject& where) const;

	enum MatchMode { AndMode, OrMode };
	enum MatchResult { NoProperty, Matched, NotMatched };

	MatchResult CheckClause(const MojObject& clause, const MojObject& response,
		MatchMode mode) const;
	MatchResult CheckClauses(const MojObject& clauses,
		const MojObject& response, MatchMode mode) const;

	/* If a key matches a property that maps to an array, rather than a
	 * specific value or object, the match proceeds to perform a DFS,
	 * ultimately iterating over all the values in the array using the
	 * current mode. */
	MatchResult CheckProperty(const MojObject& keyArray,
		MojObject::ConstArrayIterator keyIter, const MojObject& responseArray,
		MojObject::ConstArrayIterator responseIter, const MojObject& op,
		const MojObject& val, MatchMode mode) const;
	MatchResult CheckProperty(const MojObject& keyArray,
		MojObject::ConstArrayIterator keyIter, const MojObject& response,
		const MojObject& op, const MojObject& val, MatchMode mode) const;
	MatchResult CheckProperty(const MojObject& key, const MojObject& response,
		const MojObject& op, const MojObject& val, MatchMode mode) const;

	MatchResult CheckMatches(const MojObject& rhsArray, const MojObject& op,
		const MojObject& val, MatchMode mode) const;
	MatchResult CheckMatch(const MojObject& rhs, const MojObject& op,
		const MojObject& val) const;

	MojObject	m_where;
};

#endif /* __ACTIVITYMANAGER_MOJOWHEREMATCHER_H__ */
