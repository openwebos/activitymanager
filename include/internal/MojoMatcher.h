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

#ifndef __ACTIVITYMANAGER_MOJOMATCHER_H__
#define __ACTIVITYMANAGER_MOJOMATCHER_H__

#include "Base.h"

class MojoMatcher
{
public:
	MojoMatcher();
	virtual ~MojoMatcher();

	virtual bool Match(const MojObject& response) = 0;
	virtual void Reset();

	virtual MojErr ToJson(MojObject& rep, unsigned long flags) const = 0;

protected:
	static MojLogger	s_log;
};

class MojoSimpleMatcher : public MojoMatcher
{
public:
	MojoSimpleMatcher();
	virtual ~MojoSimpleMatcher();

	virtual bool Match(const MojObject& response);
	virtual void Reset();

	virtual MojErr ToJson(MojObject& rep, unsigned long flags) const;

protected:
	bool m_setupComplete;
};

/*
 * "key" : <keyname>
 */
class MojoKeyMatcher : public MojoMatcher
{
public:
	MojoKeyMatcher(const MojString& key);
	virtual ~MojoKeyMatcher();

	virtual bool Match(const MojObject& response);

	virtual MojErr ToJson(MojObject& rep, unsigned long flags) const;

protected:
	MojString	m_key;
};

/*
 * "compare" : {
 *     "key" : <string>
 *     "value" : <object>
 * }
 */
class MojoCompareMatcher : public MojoMatcher
{
public:
	MojoCompareMatcher(const MojString& key, const MojObject& value);
	virtual ~MojoCompareMatcher();

	virtual bool Match(const MojObject& response);

	virtual MojErr ToJson(MojObject& rep, unsigned long flags) const;

protected:
	MojString	m_key;
	MojObject	m_value;
};

/*
 * "where" : [{
 *     "prop" : <property name> | [{ <property name>, ... }]
 *     "op" : "<" | "<=" | "=" | ">=" | ">" | "!="
 *     "val" : <comparison value>
 * }]
 */
class MojoWhereMatcher : public MojoMatcher
{
public:
	MojoWhereMatcher(const MojObject& where);
	virtual ~MojoWhereMatcher();

	virtual bool Match(const MojObject& response);

	virtual MojErr ToJson(MojObject& rep, unsigned long flags) const;

protected:
	void ValidateKey(const MojObject& key) const;
	void ValidateOp(const MojObject& op) const;
	void ValidateClause(const MojObject& clause) const;
	void ValidateClauses(const MojObject& where) const;

	bool CheckClause(const MojObject& clause, const MojObject& response) const;
	bool CheckClauses(const MojObject& clauses,
		const MojObject& response) const;

	bool GetObjectValue(const MojObject& key,
		const MojObject& response, MojObject& value) const;

	MojObject	m_where;
};

#endif /* __ACTIVITYMANAGER_MOJOMATCHER_H__ */
