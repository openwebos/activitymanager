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

#include "MojoDBPersistCommand.h"
#include "MojoDBPersistToken.h"
#include "MojoDBProxy.h"
#include "Activity.h"
#include "ActivityJson.h"

#include <stdexcept>

/*
 * palm://com.palm.db/put
 *
 * { "objects" :
 *    [{ "_kind" : "com.palm.activity:1", "_id" : "XXX", "_rev" : 123,
 *       "prop1" : VAL1, "prop2" : VAL2 },
 *     ... ]
 * }
 */
MojoDBStoreCommand::MojoDBStoreCommand(MojService *service,
	boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
	: MojoPersistCommand(service, "palm://com.palm.db/put", activity,
		completion)
{
}

MojoDBStoreCommand::~MojoDBStoreCommand()
{
}

std::string MojoDBStoreCommand::GetMethod() const
{
	return "Store";
}

void MojoDBStoreCommand::UpdateParams(MojObject& params)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] [PersistCommand %s]: Updating "
		"parameters"), m_activity->GetId(), GetString().c_str());

	Validate(false);

	MojErr err;
	MojObject rep;
	err = m_activity->ToJson(rep,
		ACTIVITY_JSON_PERSIST | ACTIVITY_JSON_DETAIL);
	if (err) {
		throw std::runtime_error("Failed to convert Activity to JSON "
			"representation");
	}

	boost::shared_ptr<MojoDBPersistToken> pt =
		boost::dynamic_pointer_cast<MojoDBPersistToken, PersistToken>
			(m_activity->GetPersistToken());
	if (pt->IsValid()) {
		err = pt->ToJson(rep);
	}

	rep.putString(_T("_kind"), MojoDBProxy::ActivityKind);

	MojObject objectsArray;
	objectsArray.push(rep);

	params.put(_T("objects"), objectsArray);
}

void MojoDBStoreCommand::PersistResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] [PersistCommand %s]: Processing "
		"response %s"), m_activity->GetId(), GetString().c_str(),
		MojoObjectJson(response).c_str());

	try {
		Validate(false);
	} catch (const std::exception& except) {
		MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
			"Unexpected exception %s validating command"), m_activity->GetId(),
			GetString().c_str(), except.what());
		Complete(false);
		return;
	}

	if (!err) {
		MojErr err2;
		MojString id;
		MojInt64 rev;

		bool found = false;
		MojObject resultArray;

		found = response.get(_T("results"), resultArray);
		if (!found) {
			MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
				"Results of MojoDB persist command not found in response"),
				m_activity->GetId(), GetString().c_str());
			Complete(false);
			return;
		}

		if (resultArray.arrayBegin() == resultArray.arrayEnd()) {
			MojLogError(s_log, _T("MojoDB persist command returned empty "
				"result set"));
			Complete(false);
			return;
		}

		const MojObject& results = *(resultArray.arrayBegin());

		err2 = results.get(_T("id"), id, found);
		if (err2) {
			MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
				"Error retreiving _id from MojoDB persist command response"),
				m_activity->GetId(), GetString().c_str());
			Complete(false);
			return;
		}

		if (!found) {
			MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: _id "
				"not found in MojoDB persist command response"),
				m_activity->GetId(), GetString().c_str());
			Complete(false);
			return;
		}

		found = results.get(_T("rev"), rev);
		if (!found) {
			MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: _rev "
				"not found in MojoDB persist command response"),
				m_activity->GetId(), GetString().c_str());
			Complete(false);
			return;
		}

		boost::shared_ptr<MojoDBPersistToken> pt =
			boost::dynamic_pointer_cast<MojoDBPersistToken, PersistToken>
				(m_activity->GetPersistToken());

		try {
			if (!pt->IsValid()) {
				pt->Set(id, rev);
			} else {
				pt->Update(id, rev);
			}
		} catch (...) {
			MojLogError(s_log, _T("[Activity %llu] [PersistCommand %s]: "
				"Failed to set or update value of persist token"),
				m_activity->GetId(), GetString().c_str());
			Complete(false);
			return;
		}
	}

	MojoPersistCommand::PersistResponse(msg, response, err);
}

/*
 * palm://com.palm.db/del
 *
 * { "ids" : [{ "XXX", "XXY", "XXZ" }] }
 */
MojoDBDeleteCommand::MojoDBDeleteCommand(MojService *service,
	boost::shared_ptr<Activity> activity,
	boost::shared_ptr<Completion> completion)
	: MojoPersistCommand(service, "palm://com.palm.db/del", activity,
		completion)
{
}

MojoDBDeleteCommand::~MojoDBDeleteCommand()
{
}

std::string MojoDBDeleteCommand::GetMethod() const
{
	return "Delete";
}

void MojoDBDeleteCommand::UpdateParams(MojObject& params)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] [PersistCommand %s]: Updating "
		"parameters"), m_activity->GetId(), GetString().c_str());

	Validate(true);

	boost::shared_ptr<MojoDBPersistToken> pt =
		boost::dynamic_pointer_cast<MojoDBPersistToken, PersistToken>
			(m_activity->GetPersistToken());

	MojObject idsArray;
	idsArray.push(pt->GetId());

	params.put(_T("ids"), idsArray);
}

void MojoDBDeleteCommand::PersistResponse(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, _T("[Activity %llu] [PersistCommand %s]: Processing "
		"response %s"), m_activity->GetId(), GetString().c_str(),
		MojoObjectJson(response).c_str());

	if (!err) {
		boost::shared_ptr<MojoDBPersistToken> pt =
			boost::dynamic_pointer_cast<MojoDBPersistToken, PersistToken>
				(m_activity->GetPersistToken());

		pt->Clear();
	}

	MojoPersistCommand::PersistResponse(msg, response, err);
}

