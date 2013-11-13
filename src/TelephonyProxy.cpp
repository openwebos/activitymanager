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

#include "TelephonyProxy.h"
#include "Activity.h"
#include "MojoCall.h"
#include "Logging.h"
#include <stdexcept>

MojLogger TelephonyProxy::s_log(_T("activitymanager.telephonyproxy"));

TelephonyProxy::TelephonyProxy(MojService *service)
	: m_service(service)
    , m_haveTelephonyService(false)
{
	m_telephonyRequirementCore =
		boost::make_shared<RequirementCore>("telephony", true);
}

TelephonyProxy::~TelephonyProxy()
{
}

const std::string& TelephonyProxy::GetName() const
{
	static std::string name("TelephonyProxy");
	return name;
}

boost::shared_ptr<Requirement> TelephonyProxy::InstantiateRequirement(
	boost::shared_ptr<Activity> activity, const std::string& name,
	const MojObject& value)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Instantiating [Requirement %s] for [Activity %llu]",
		name.c_str(), activity->GetId());

	if (name == "telephony") {
		if ((value.type() == MojObject::TypeBool) && value.boolValue()) {
			boost::shared_ptr<ListedRequirement> req =
				boost::make_shared<BasicCoreListedRequirement>(
					activity, m_telephonyRequirementCore,
					m_telephonyRequirementCore->IsMet());
			m_telephonyRequirements.push_back(*req);
			return req;
		} else {
			throw std::runtime_error("If a 'telephony' requirement is "
				"specified, the only legal value is 'true'");
		}
	} else {
		LOG_ERROR(MSGID_TIL_UNKNOWN_REQ, 3, PMLOGKS("MANAGER", GetName().c_str()),
			PMLOGKS("REQ",name.c_str()),
			PMLOGKFV("ACTIVITY_ID","%llu",activity->GetId()),
			"does not know how to instantiate Requirement" );
		throw std::runtime_error("Attempt to instantiate unknown requirement");
	}
}

void TelephonyProxy::RegisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Registering requirements");

	master->RegisterRequirement("telephony", shared_from_this());
}

void TelephonyProxy::UnregisterRequirements(
	boost::shared_ptr<MasterRequirementManager> master)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Unregistering requirements");

	master->UnregisterRequirement("telephony", shared_from_this());
}

void TelephonyProxy::Enable()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Enabling TIL Proxy");

	MojObject params;
	params.putBool(_T("subscribe"), true);

	// Start out with a platformQuery
	m_platformQuery = boost::make_shared<MojoWeakPtrCall<TelephonyProxy> >(
		boost::dynamic_pointer_cast<TelephonyProxy, RequirementManager>
			(shared_from_this()),
		&TelephonyProxy::PlatformQueryUpdate, m_service,
		"palm://com.palm.telephony/platformQuery", params,
		MojoCall::Unlimited);
	m_platformQuery->Call();
}

void TelephonyProxy::Disable()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Disabling TIL Proxy");

	m_networkStatusQuery.reset();
	m_platformQuery.reset();
}

/*
 * networkStatusQuery
 *
 * {
 *    "extended" : {
 *    ...
 *        "state" : "noservice" | "limited" | "service"
 *    ...
 *    }
 * }
 *
 */
void TelephonyProxy::NetworkStatusUpdate(MojServiceMessage *msg,
	const MojObject& response, MojErr err)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	LOG_DEBUG("Network status update message: %s",
		MojoObjectJson(response).c_str());

	if (err != MojErrNone) {
		if (MojoCall::IsPermanentFailure(msg, response, err)) {
			LOG_WARNING(MSGID_TIL_NWSTATUS_QUERY_ERR,0,
				"TIL experienced an uncorrectable failure : %s", MojoObjectJson(response).c_str());
			m_networkStatusQuery.reset();
		} else {
			LOG_WARNING(MSGID_TIL_NWSTATUS_QUERY_RETRY, 0,
				"Subscription to TIL failed, retrying - %s", MojoObjectJson(response).c_str() );
			m_networkStatusQuery->Call();
		}
		return;
	}

	MojObject extended;

	bool found;
	found = response.get(_T("extended"), extended);
	if (!found) {
		/* Way to have 2 different formats for the same thing!  First
		 * response names the property "extended".  All subsequent ones
		 * name it "eventNetwork". */
		found = response.get(_T("eventNetwork"), extended);
	}

	if (found) {
		MojString status;
		MojErr err2;

		err2 = extended.get(_T("state"), status, found);
		if (err2) {
			LOG_WARNING(MSGID_TIL_NWSTATE_ERR, 1 ,
				PMLOGKFV("NWSTATE","%d",err2),
				"Error attempting to retrieve network state");
		} else if (found) {
			if (status == "service") {
			    m_haveTelephonyService = true;
			} else {
			    m_haveTelephonyService = false;
			}
			UpdateTelephonyState();
		} else {
			LOG_WARNING(MSGID_TIL_NO_NWSTATE, 0 ,
				"Network status update message did not include network state: %s", MojoObjectJson(response).c_str());
		}
	}
}

/*
 * {
 *    "extended": {
 *        "capabilities" : {
 *            ...
 *            "mapcenable" : true | false
 *            ...
 *        }
 *    }
 * }
 *
 */
void TelephonyProxy::PlatformQueryUpdate(MojServiceMessage *msg,
    const MojObject& response, MojErr err)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    LOG_DEBUG("Platform query update message: %s",
        MojoObjectJson(response).c_str());

    if (err != MojErrNone) {
        if (MojoCall::IsPermanentFailure(msg, response, err)) {
            LOG_WARNING(MSGID_TIL_QUERYUPDATE_ERR,0,
				"experienced an uncorrectable failure: %s", MojoObjectJson(response).c_str());
            m_platformQuery.reset();
        } else {
            LOG_WARNING(MSGID_TIL_QUERYUPDATE_RETRY, 0,
				"Platform query subscription to TIL failed, retrying: %s",
                MojoObjectJson(response).c_str());
            m_platformQuery->Call();
        }
        return;
    }

    MojObject extended;
    bool found;

    found = response.get(_T("extended"), extended);
    if (!found) {
        LOG_WARNING(MSGID_TIL_NO_EXTENDED, 0, "Platform query update did not contain extended");
        return;
    }

    MojObject capabilities;

    found = extended.get(_T("capabilities"), capabilities);
    if (!found) {
        LOG_WARNING(MSGID_TIL_NO_CAPABILITIES, 0, "Platform query update did not contain capabilities");
        return;
    }

    bool mapcenable;

    found = capabilities.get(_T("mapcenable"), mapcenable);

    if (found) {
        if (mapcenable) {
            m_haveTelephonyService = true;
        } else {
            m_haveTelephonyService = false;
        }
        UpdateTelephonyState();
    } else {
        LOG_WARNING(MSGID_TIL_NO_MAPCE_ENABLE, 0,
            "message did not include mapcenable: %s", MojoObjectJson(response).c_str());

        // Disable platformQuery and fall back to networkStatusQuery
        m_platformQuery.reset();

        MojObject params;
        params.putBool(_T("subscribe"), true);

        m_networkStatusQuery = boost::make_shared<MojoWeakPtrCall<TelephonyProxy> >(
        		boost::dynamic_pointer_cast<TelephonyProxy, RequirementManager>
        			(shared_from_this()),
        		&TelephonyProxy::NetworkStatusUpdate, m_service,
        		"palm://com.palm.telephony/networkStatusQuery", params,
        		MojoCall::Unlimited);
        m_networkStatusQuery->Call();
    }
}

void TelephonyProxy::UpdateTelephonyState()
{
    if (m_haveTelephonyService) {
		if (!m_telephonyRequirementCore->IsMet()) {
			LOG_DEBUG("Telephony service available");
			m_telephonyRequirementCore->Met();
			std::for_each(m_telephonyRequirements.begin(),
				m_telephonyRequirements.end(),
				boost::mem_fn(&Requirement::Met));
		}
    } else {
		if (m_telephonyRequirementCore->IsMet()) {
			LOG_DEBUG("Telephony service unavailable");
			m_telephonyRequirementCore->Unmet();
			std::for_each(m_telephonyRequirements.begin(),
				m_telephonyRequirements.end(),
				boost::mem_fn(&Requirement::Unmet));
		}
    }
}
