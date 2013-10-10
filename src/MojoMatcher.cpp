// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// LICENSE@@@

#include "MojoMatcher.h"

#include <stdexcept>

MojLogger MojoMatcher::s_log("activitymanager.triggermatcher");

MojoMatcher::MojoMatcher()
{
}

MojoMatcher::~MojoMatcher()
{
}

void MojoMatcher::Reset()
{
}

MojoSimpleMatcher::MojoSimpleMatcher()
	: m_setupComplete(false)
{
}

MojoSimpleMatcher::~MojoSimpleMatcher()
{
}

bool MojoSimpleMatcher::Match(const MojObject& response)
{
	if (!m_setupComplete) {
		MojLogDebug(s_log, _T("Simple Matcher: Setup successfully"));
		m_setupComplete = true;
		return false;
	} else {
		MojLogDebug(s_log, _T("Simple Matcher: Matched"));
		return true;
	}
}

void MojoSimpleMatcher::Reset()
{
	MojLogDebug(s_log, _T("Simple Matcher: Resetting"));

	m_setupComplete = false;
}

MojErr MojoSimpleMatcher::ToJson(MojObject& rep, unsigned long flags) const
{
	return MojErrNone;
}

MojoKeyMatcher::MojoKeyMatcher(const MojString& key)
	: m_key(key)
{
}

MojoKeyMatcher::~MojoKeyMatcher()
{
}

bool MojoKeyMatcher::Match(const MojObject& response)
{
	MojLogTrace(s_log);

	/* If those were the droids we were looking for, fire! */
	if (response.contains(m_key)) {
		MojLogDebug(s_log, _T("Key Matcher: Key \"%s\" found in response %s"),
			m_key.data(), MojoObjectJson(response).c_str());
		return true;
	} else {
		MojLogDebug(s_log, _T("Key Matcher: Key \"%s\" not found in response"
			"%s"), m_key.data(), MojoObjectJson(response).c_str());
		return false;
	}
}

MojErr MojoKeyMatcher::ToJson(MojObject& rep, unsigned long flags) const
{
	MojErr err;

	err = rep.put(_T("key"), m_key);
	MojErrCheck(err);

	return MojErrNone;
}

MojoCompareMatcher::MojoCompareMatcher(const MojString& key,
	const MojObject& value)
	: m_key(key)
	, m_value(value)
{
}

MojoCompareMatcher::~MojoCompareMatcher()
{
}

bool MojoCompareMatcher::Match(const MojObject& response)
{
	MojLogTrace(s_log);

	if (response.contains(m_key)) {
		MojObject value;
		MojString valueString;
		response.get(m_key, value);
		value.stringValue(valueString);
		if (value != m_value) {
			MojString oldValueString;
			m_value.stringValue(oldValueString);

			MojLogDebug(s_log, _T("Compare Matcher: Comparison key \"%s\" "
				"value changed from \"%s\" to \"%s\".  Firing."), m_key.data(),
				oldValueString.data(), valueString.data());

			return true;
		} else {
			MojLogDebug(s_log, _T("Compare Matcher: Comparison key \"%s\" "
				"value \"%s\" unchanged."), m_key.data(), valueString.data());
		}
	} else {
		MojLogDebug(s_log, _T("Compare Matcher: Comparison key (%s) not "
			"present."), m_key.data());
	}

	return false;
}


MojErr MojoCompareMatcher::ToJson(MojObject& rep, unsigned long flags) const
{
	MojErr err;

	MojObject compare;

	err = compare.put(_T("key"), m_key);
	MojErrCheck(err);

	err = compare.put(_T("value"), m_value);
	MojErrCheck(err);

	err = rep.put(_T("compare"), compare);
	MojErrCheck(err);

	return MojErrNone;
}

MojoWhereMatcher::MojoWhereMatcher(const MojObject& where)
	: m_where(where)
{
	ValidateClauses(m_where);
}

MojoWhereMatcher::~MojoWhereMatcher()
{
}

bool MojoWhereMatcher::Match(const MojObject& response)
{
	MojLogTrace(s_log);

	if (CheckClauses(m_where, response)) {
		MojLogDebug(s_log, _T("Where Matcher: Response %s matches"),
			MojoObjectJson(response).c_str());
		return true;
	} else {
		MojLogDebug(s_log, _T("Where Matcher: Response %s does not match"),
			MojoObjectJson(response).c_str());
		return false;
	}
}

MojErr MojoWhereMatcher::ToJson(MojObject& rep, unsigned long flags) const
{
	MojErr err;

	err = rep.put(_T("where"), m_where);
	MojErrCheck(err);

	return MojErrNone;
}


void MojoWhereMatcher::ValidateKey(const MojObject& key) const
{
	if (key.type() == MojObject::TypeArray) {
		for (MojObject::ConstArrayIterator iter = key.arrayBegin();
			iter != key.arrayEnd(); ++iter) {
			const MojObject& keyObj = *iter;
			if (keyObj.type() != MojObject::TypeString) {
				throw std::runtime_error("Something other than a string "
					"found in array of property names");
			}
		}
	} else if (key.type() != MojObject::TypeString) {
		throw std::runtime_error("Property keys must be specified as "
			"a property name, or array of property names");
	}
}

void MojoWhereMatcher::ValidateOp(const MojObject& op) const
{
	if (op.type() != MojObject::TypeString) {
		throw std::runtime_error("Operation must be specified as a string "
			"property");
	}

	MojString opStr;
	MojErr err = op.stringValue(opStr);
	if (err) {
		throw std::runtime_error("Failed to convert operation to "
			"string value");
	}

	if ((opStr != "<") && (opStr != "<=") && (opStr != "=") &&
		(opStr != ">=") && (opStr != ">") && (opStr != "!=")) {
		throw std::runtime_error("Operation must be one of '<', '<=', "
			"'=', '>=', '>', and '!='");
	}
}

void MojoWhereMatcher::ValidateClause(const MojObject& clause) const
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Validating where clause \"%s\""),
		MojoObjectJson(clause).c_str());

	if (!clause.contains(_T("prop"))) {
		throw std::runtime_error("Each where clause must contain a property "
			"to operate on");
	}

	MojObject prop;
	clause.get(_T("prop"), prop);
	ValidateKey(prop);

	if (!clause.contains(_T("op"))) {
		throw std::runtime_error("Each where clause must contain a test "
			"operation to perform");
	}

	MojObject op;
	clause.get(_T("op"), op);
	ValidateOp(op);

	if (!clause.contains(_T("val"))) {
		throw std::runtime_error("Each where clause must contain a value to "
			"test against");
	}
}

void MojoWhereMatcher::ValidateClauses(const MojObject& where) const
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Validating trigger clauses"));

	if (where.type() == MojObject::TypeObject) {
		ValidateClause(where);
	} else if (where.type() == MojObject::TypeArray) {
		for (MojObject::ConstArrayIterator iter = where.arrayBegin();
			iter != where.arrayEnd(); ++iter) {
			const MojObject& clause = *iter;
			if (clause.type() != MojObject::TypeObject) {
				throw std::runtime_error("where statement array must "
					"consist of valid clauses");
			} else {
				ValidateClause(clause);
			}
		}
	} else {
		throw std::runtime_error("where statement should consist of a single "
			"clause or array of valid clauses");
	}
}

bool MojoWhereMatcher::CheckClause(const MojObject& clause,
	const MojObject& response) const
{
	MojLogTrace(s_log);
	bool result;

	if (clause.type() != MojObject::TypeObject) {
		throw std::runtime_error("Clause must be an object with prop, op, "
			"and val properties");
	}

	bool found;
	MojObject prop;
	found = clause.get(_T("prop"), prop);
	if (!found) {
		throw std::runtime_error("Property specifier not present in where "
			"clause");
	}

	/* Value not present causes clause to be skipped.
	 * XXX allow missing behaviour to be specified? */
	MojObject propValue;
	found = GetObjectValue(prop, response, propValue);
	if (found) {
		MojObject clauseValue;
		found = clause.get(_T("val"), clauseValue);
		if (!found) {
			throw std::runtime_error("Value to compare against not present in "
				"where clause");
		}

		found = false;
		MojString op;
		MojErr err = clause.get(_T("op"), op, found);
		if (err) {
			throw std::runtime_error("Error retrieving operation to use for "
				"where clause");
		} else if (!found) {
			throw std::runtime_error("operation to use for comparison not "
				"present in where clause");
		}

		if (op == "<") {
			result = (propValue < clauseValue);
		} else if (op == "<=") {
			result = (propValue <= clauseValue);
		} else if (op == "=") {
			result = (propValue == clauseValue);
		} else if (op == "!=") {
			result = (propValue != clauseValue);
		} else if (op == ">=") {
			result = (propValue >= clauseValue);
		} else if (op == ">") {
			result = (propValue > clauseValue);
		} else {
			throw std::runtime_error("Unknown comparison operator in where "
				"clause");
		}
	} else {
		result = false;
	}

	MojLogDebug(s_log, _T("Where Trigger: Clause %s %s"),
		MojoObjectJson(clause).c_str(), result ? "matched" : "did not match");

	return result;
}

bool MojoWhereMatcher::CheckClauses(const MojObject& clauses,
	const MojObject& response) const
{
	MojLogTrace(s_log);

	if (clauses.type() == MojObject::TypeObject) {
		return CheckClause(clauses, response);
	} else if (clauses.type() == MojObject::TypeArray) {
		for (MojObject::ConstArrayIterator iter = clauses.arrayBegin();
			iter != clauses.arrayEnd(); ++iter) {
			if (!CheckClause(*iter, response)) {
				return false;
			}
		}
	} else {
		throw std::runtime_error("Clauses must consist of either a single "
			"where clause or array of where clauses");
	}

	return true;
}

bool MojoWhereMatcher::GetObjectValue(const MojObject& key,
	const MojObject& response, MojObject& value) const
{
	if (key.type() == MojObject::TypeString) {
		MojString keyStr;
		MojErr err = key.stringValue(keyStr);
		if (err) {
			throw std::runtime_error("Error attempting to retrieve key name "
				"getting object value");
		}

		if (response.contains(keyStr)) {
			return response.get(keyStr, value);
		} else {
			return false;
		}
	} else if (key.type() == MojObject::TypeArray) {
		MojObject cur = response;
		for (MojObject::ConstArrayIterator iter = key.arrayBegin();
			iter != key.arrayEnd(); ++iter) {
			const MojObject& keyObj = *iter; 

			if (keyObj.type() != MojObject::TypeString) {
				throw std::runtime_error("Non-string found in property "
					"specifiction array");
			}

			MojString keyStr;
			MojErr err = keyObj.stringValue(keyStr);
			if (err) {
				throw std::runtime_error("Failed to access lookup key");
			}

			if (cur.contains(keyStr)) {
				MojObject tmp;
				if (!cur.get(keyStr, tmp)) {
					return false;
				} else {
					cur = tmp;
				}
			} else {
				return false;
			}
		}

		value = cur;
		return true;
	} else {
		throw std::runtime_error("Property specification must be the "
			"property name or array of property names");
	}
}

