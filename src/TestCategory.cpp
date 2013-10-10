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

#include "TestCategory.h"
#include "Category.h"
#include "MojoJsonConverter.h"
#include "MojoSubscription.h"
#include "MojoWhereMatcher.h"
#include "Activity.h"

#include <stdexcept>

// TODO: I could not call these methods, so leaving them out of the generated documentation
/* !
 * \page com_palm_activitymanager_test Service API com.palm.activitymanager/test/
 * Private methods:
 * - \ref com_palm_activitymanager_test_leak
 * - \ref com_palm_activitymanager_test_where
 */

const TestCategoryHandler::Method TestCategoryHandler::s_methods[] = {
	{ _T("leak"), (Callback) &TestCategoryHandler::Leak },
	{ _T("where"), (Callback) &TestCategoryHandler::WhereMatchTest },
	{ NULL, NULL }
};

MojLogger TestCategoryHandler::s_log(_T("activitymanager.testhandler"));

TestCategoryHandler::TestCategoryHandler(
	boost::shared_ptr<MojoJsonConverter> json)
	: m_json(json)
{
}

TestCategoryHandler::~TestCategoryHandler()
{
}

MojErr TestCategoryHandler::Init()
{
	MojErr err;

	err = addMethods(s_methods);
	MojErrCheck(err);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager_test
\n
\section com_palm_activitymanager_test_leak leak

\e Private.

com.palm.activitymanager/test/leak

Grab an extra reference to an Activity (to force it to leak).

\subsection com_palm_activitymanager_test_leak_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_test_leak_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_test_leak_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/test/leak '{ }'
\endcode

Example response for a succesful call:
\code
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
TestCategoryHandler::Leak(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("Leak: %s"), MojoObjectJson(payload).c_str());

	MojErr err = MojErrNone;

	bool freeAll = false;

	payload.get(_T("freeAll"), freeAll);

	if (freeAll) {
		MojLogCritical(s_log, _T("RELEASING REFERENCES TO ALL INTENTIONALLY "
			"LEAKED ACTIVITIES"));
		while (!m_leakedActivities.empty()) {
			boost::shared_ptr<Activity> act = m_leakedActivities.front();
			m_leakedActivities.pop_front();

			MojLogCritical(s_log, _T("RELEASING REFERENCE TO [Activity %llu]"),
				act->GetId());
		}
	} else {
		boost::shared_ptr<Activity> act;

		err = LookupActivity(msg, payload, act);
		MojErrCheck(err);

		MojLogCritical(s_log, _T("INTENTIONALLY LEAKING [Activity %llu]!!"),
			act->GetId());

		m_leakedActivities.push_back(act);
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

/* !
\page com_palm_activitymanager_test
\n
\section com_palm_activitymanager_test_where where

\e Private.

com.palm.activitymanager/test/where

Wake Callback.

\subsection com_palm_activitymanager_test_where_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_test_where_returns Returns:
\code

\endcode

\param

\subsection com_palm_activitymanager_test_where_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/test/where '{ }'
\endcode

Example response for a succesful call:
\code
\endcode

Example response for a failed call:
\code
\endcode
*/

MojErr
TestCategoryHandler::WhereMatchTest(MojServiceMessage *msg, MojObject &payload)
{
	ACTIVITY_SERVICEMETHOD_BEGIN();

	MojObject response;
	bool found = payload.get(_T("response"), response);
	if (!found) {
		throw std::runtime_error("Please provide a \"response\" and a "
			"\"where\" clause to test against it");
	}

	MojObject where;
	found = payload.get(_T("where"), where);
	if (!found) {
		throw std::runtime_error("Please provide a \"where\" clause to test "
			"the response against");
	}

	MojoNewWhereMatcher matcher(where);
	bool matched = matcher.Match(response);

	MojObject reply;
	MojErr err = reply.putBool(_T("matched"), matched);
	MojErrCheck(err);

	err = msg->reply(reply);
	MojErrCheck(err);

	ACTIVITY_SERVICEMETHOD_END(msg);

	return MojErrNone;
}

MojErr
TestCategoryHandler::LookupActivity(MojServiceMessage *msg, MojObject& payload, boost::shared_ptr<Activity>& act)
{
	try {
		act = m_json->LookupActivity(payload, MojoSubscription::GetBusId(msg));
	} catch (const std::exception& except) {
		msg->replyError(MojErrNotFound, except.what());
		return MojErrNotFound;
	}

	return MojErrNone;
}

