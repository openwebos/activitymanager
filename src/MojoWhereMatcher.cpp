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

#include "MojoWhereMatcher.h"

#include <stdexcept>

MojoNewWhereMatcher::MojoNewWhereMatcher(const MojObject& where)
	: m_where(where)
{
	ValidateClauses(m_where);
}

MojoNewWhereMatcher::~MojoNewWhereMatcher()
{
}

bool MojoNewWhereMatcher::Match(const MojObject& response)
{
	MojLogTrace(s_log);

	MatchResult result = CheckClause(m_where, response, AndMode);
	if (result == Matched) {
		MojLogDebug(s_log, _T("Where Matcher: Response %s matches"),
			MojoObjectJson(response).c_str());
		return true;
	} else {
		MojLogDebug(s_log, _T("Where Matcher: Response %s does not match"),
			MojoObjectJson(response).c_str());
		return false;
	}
}

MojErr MojoNewWhereMatcher::ToJson(MojObject& rep, unsigned long flags) const
{
	MojErr err;

	err = rep.put(_T("where"), m_where);
	MojErrCheck(err);

	return MojErrNone;
}


void MojoNewWhereMatcher::ValidateKey(const MojObject& key) const
{
	if (key.type() == MojObject::TypeArray) {
		for (MojObject::ConstArrayIterator iter = key.arrayBegin();
			iter != key.arrayEnd(); ++iter) {
			const MojObject& keyObj = *iter;
			if (keyObj.type() != MojObject::TypeString) {
				throw std::runtime_error("Something other than a string "
					"found in the key array of property names");
			}
		}
	} else if (key.type() != MojObject::TypeString) {
		throw std::runtime_error("Property keys must be specified as "
			"a property name, or array of property names");
	}
}

void MojoNewWhereMatcher::ValidateOp(const MojObject& op,
	const MojObject& val) const
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
		(opStr != ">=") && (opStr != ">") && (opStr != "!=") &&
		(opStr != "where")) {
		throw std::runtime_error("Operation must be one of '<', '<=', "
			"'=', '>=', '>', '!=', and 'where'");
	}

	if (opStr == "where") {
		ValidateClauses(val);
	}
}

void MojoNewWhereMatcher::ValidateClause(const MojObject& clause) const
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Validating where clause \"%s\""),
		MojoObjectJson(clause).c_str());

	bool found = false;

	if (clause.contains(_T("and"))) {
		found = true;

		MojObject andClauses;
		clause.get(_T("and"), andClauses);
		ValidateClauses(andClauses);
	}

	if (clause.contains(_T("or"))) {
		if (found) {
			throw std::runtime_error("Only one of \"and\", \"or\", or a valid "
				"clause including \"prop\", \"op\", and a \"val\"ue to "
				"compare against must be present in a clause");
		}

		found = true;

		MojObject orClauses;
		clause.get(_T("or"), orClauses);
		ValidateClauses(orClauses);
	}

	if (!clause.contains(_T("prop"))) {
		if (!found) {
			throw std::runtime_error("Each where clause must contain \"or\", "
				"\"and\", or a \"prop\"erty to compare against");
		} else {
			return;
		}
	} else if (found) {
		throw std::runtime_error("Only one of \"and\", \"or\", or a valid "
			"clause including \"prop\", \"op\", and a \"val\"ue to "
			"compare against must be present in a clause");
	}

	MojObject prop;
	clause.get(_T("prop"), prop);
	ValidateKey(prop);

	if (!clause.contains(_T("val"))) {
		throw std::runtime_error("Each where clause must contain a value to "
			"test against");
	}

	MojObject val;
	clause.get(_T("val"), val);

	if (!clause.contains(_T("op"))) {
		throw std::runtime_error("Each where clause must contain a test "
			"operation to perform");
	}

	MojObject op;
	clause.get(_T("op"), op);
	ValidateOp(op, val);
}

void MojoNewWhereMatcher::ValidateClauses(const MojObject& where) const
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

MojoNewWhereMatcher::MatchResult MojoNewWhereMatcher::CheckClauses(
	const MojObject& clauses, const MojObject& response, MatchMode mode) const
{
	MojLogTrace(s_log);

	if (clauses.type() == MojObject::TypeObject) {
		return CheckClause(clauses, response, mode);
	} else if (clauses.type() != MojObject::TypeArray) {
		throw std::runtime_error("Multiple clauses must be specified as an "
			"array of clauses");
	}

	MojLogDebug(s_log, _T("Checking clauses '%s' against response '%s' (%s)"),
		MojoObjectJson(clauses).c_str(), MojoObjectJson(response).c_str(),
		(mode == AndMode) ? "and" : "or");

	for (MojObject::ConstArrayIterator iter = clauses.arrayBegin();
		iter != clauses.arrayEnd(); ++iter) {
		MatchResult result = CheckClause(*iter, response, mode);

		if (mode == AndMode) {
			if (result != Matched) {
				return NotMatched;
			}
		} else {
			if (result == Matched) {
				return Matched;
			}
		}
	}

	if (mode == AndMode) {
		return Matched;
	} else {
		return NotMatched;
	}
}

MojoNewWhereMatcher::MatchResult MojoNewWhereMatcher::CheckClause(
	const MojObject& clause, const MojObject& response, MatchMode mode) const
{
	MojLogTrace(s_log);

	if (clause.type() == MojObject::TypeArray) {
		return CheckClauses(clause, response, mode);
	} else if (clause.type() != MojObject::TypeObject) {
		throw std::runtime_error("Clauses must be either an object or array "
			"of objects");
	}

	MojLogDebug(s_log, _T("Checking clause '%s' against response '%s' (%s)"),
		MojoObjectJson(clause).c_str(), MojoObjectJson(response).c_str(),
		(mode == AndMode) ? "and" : "or");

	if (clause.contains(_T("and"))) {
		MojObject andClause;
		clause.get(_T("and"), andClause);

		return CheckClause(andClause, response, AndMode);
	} else if (clause.contains(_T("or"))) {
		MojObject orClause;
		clause.get(_T("or"), orClause);

		return CheckClause(orClause, response, OrMode);
	}

	MojObject prop;
	bool found = clause.get(_T("prop"), prop);
	if (!found) {
		throw std::runtime_error("Clauses must contain \"and\", \"or\", "
			"or a comparison to make");
	}

	MojObject op;
	found = clause.get(_T("op"), op);
	if (!found) {
		throw std::runtime_error("Clauses must specify a comparison operation "
			"to perform");
	}

	MojObject val;
	found = clause.get(_T("val"), val);
	if (!found) {
		throw std::runtime_error("Clauses must specify a value to compare "
			"against");
	}

	MatchResult result = CheckProperty(prop, response, op, val, mode);

	MojLogDebug(s_log, _T("Where Trigger: Clause %s %s"),
		MojoObjectJson(clause).c_str(), (result == Matched) ? "matched" :
			"did not match");

	return result;
}

MojoNewWhereMatcher::MatchResult MojoNewWhereMatcher::CheckProperty(
	const MojObject& keyArray, MojObject::ConstArrayIterator keyIter,
	const MojObject& responseArray, MojObject::ConstArrayIterator responseIter,
	const MojObject& op, const MojObject& val, MatchMode mode) const
{
	/* Yes, this will iterate into arrays of arrays of arrays */
	for (; responseIter != responseArray.arrayEnd(); ++responseIter) {
		MatchResult result = CheckProperty(keyArray, keyIter, *responseIter,
			op, val, mode);

		if (mode == AndMode) {
			if (result != Matched) {
				return NotMatched;
			}
		} else {
			if (result == Matched) {
				return Matched;
			}
		}
	}

	if (mode == AndMode) {
		return Matched;
	} else {
		return NotMatched;
	}
}

MojoNewWhereMatcher::MatchResult MojoNewWhereMatcher::CheckProperty(
	const MojObject& keyArray, MojObject::ConstArrayIterator keyIter,
	const MojObject& response, const MojObject& op, const MojObject& val,
	MatchMode mode) const
{
	MojObject onion = response;
	for (; keyIter != keyArray.arrayEnd(); ++keyIter) {
		if (onion.type() == MojObject::TypeArray) {
			return CheckProperty(keyArray, keyIter, onion, onion.arrayBegin(),
				op, val, mode);
		} else if (onion.type() == MojObject::TypeObject) {
			MojString keyStr;
			MojErr err = (*keyIter).stringValue(keyStr);
			if (err) {
				throw std::runtime_error("Failed to convert property lookup "
					"key to string");
			}

			MojObject next;
			if (!onion.get(keyStr.data(), next)) {
				return NoProperty;
			}

			onion = next;
		} else {
			return NoProperty;
		}
	}

	return CheckMatch(onion, op, val);

}

MojoNewWhereMatcher::MatchResult MojoNewWhereMatcher::CheckProperty(
	const MojObject& key, const MojObject& response, const MojObject& op,
	const MojObject& val, MatchMode mode) const
{
	if (key.type() == MojObject::TypeString) {
		MojString keyStr;
		MojErr err = key.stringValue(keyStr);
		if (err) {
			throw std::runtime_error("Failed to convert property lookup key "
				"to string");
		}

		MojObject propVal;
		bool found = response.get(keyStr.data(), propVal);
		if (!found) {
			return NoProperty;
		}

		return CheckMatch(propVal, op, val);

	} else if (key.type() == MojObject::TypeArray) {
		return CheckProperty(key, key.arrayBegin(), response, op, val, mode);
	} else {
		throw std::runtime_error("Key specified was neither a string or "
			"array of strings");
	}
}

MojoNewWhereMatcher::MatchResult MojoNewWhereMatcher::CheckMatches(
	const MojObject& rhsArray, const MojObject& op, const MojObject& val,
	MatchMode mode) const
{
	/* Matching a value against an array */
	for (MojObject::ConstArrayIterator iter = rhsArray.arrayBegin();
		iter != rhsArray.arrayEnd(); ++iter) {
		MatchResult result = CheckMatch(*iter, op, val);
		if (mode == AndMode) {
			if (result != Matched) {
				return NotMatched;
			}
		} else {
			if (result == Matched) {
				return Matched;
			}
		}
	}

	/* If we got here in And mode it means all the values matched.  If we
	 * got here in Or mode, it means none of them did. */
	if (mode == AndMode) {
		return Matched;
	} else {
		return NotMatched;
	}
}

MojoNewWhereMatcher::MatchResult MojoNewWhereMatcher::CheckMatch(
	const MojObject& rhs, const MojObject& op, const MojObject& val) const
{
	MojString opStr;
	MojErr err = op.stringValue(opStr);
	if (err) {
		throw std::runtime_error("Failed to convert operation to string "
			"value");
	}

	bool result;

	if (opStr == "<") {
		result = (rhs < val);
	} else if (opStr == "<=") {
		result = (rhs <= val);
	} else if (opStr == "=") {
		result = (rhs == val);
	} else if (opStr == "!=") {
		result = (rhs != val);
	} else if (opStr == ">=") {
		result = (rhs >= val);
	} else if (opStr == ">") {
		result = (rhs > val);
	} else if (opStr == "where") {
		result = CheckClause(val, rhs, AndMode);
	} else {
		throw std::runtime_error("Unknown comparison operator in where "
			"clause");
	}

	if (result) {
		return Matched;
	} else {
		return NotMatched;
	}
}

