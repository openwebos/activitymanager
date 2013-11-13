/* @@@LICENSE
 *
 *      Copyright (c) 2011-2013 LG Electronics, Inc.
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
 * LICENSE@@@
 */

#ifndef __ACTIVITYMANAGER_LOGGING_H__
#define __ACTIVITYMANAGER_LOGGING_H__

#include <PmLogLib.h>

/* Logging for ActivityManager context ********
 * The parameters needed are
 * msgid - unique message id
 * kvcount - count for key-value pairs
 * ... - key-value pairs and free text. key-value pairs are formed using PMLOGKS or PMLOGKFV
 * e.g.)
 * LOG_CRITICAL(msgid, 2, PMLOGKS("key1", "value1"), PMLOGKFV("key2", "%d", value2), "free text message");
 **********************************************/
#define LOG_CRITICAL(msgid, kvcount, ...) \
        PmLogCritical(getactivitymanagercontext(), msgid, kvcount, ##__VA_ARGS__)

#define LOG_ERROR(msgid, kvcount, ...) \
        PmLogError(getactivitymanagercontext(), msgid, kvcount,##__VA_ARGS__)

#define LOG_WARNING(msgid, kvcount, ...) \
        PmLogWarning(getactivitymanagercontext(), msgid, kvcount, ##__VA_ARGS__)

#define LOG_INFO(msgid, kvcount, ...) \
        PmLogInfo(getactivitymanagercontext(), msgid, kvcount, ##__VA_ARGS__)

#define LOG_DEBUG(...) \
        PmLogDebug(getactivitymanagercontext(), ##__VA_ARGS__)

#define LOG_TRACE(...) \
        PMLOG_TRACE(__VA_ARGS__);


/** list of MSGID's pairs */
/** activity.c */
#define MSGID_ACTIVITIES_LOAD_ERR                       "ACTIVITIES_LOAD_ERR" /** failed to load activities from Db */
#define MSGID_ACTIVITYID_NOT_FOUND                      "ACTIVITYID_NOT_FOUND" /** Could not find activityId */
#define MSGID_ACTIVITY_REPLACED                         "ACTIVITY_REPLACED" /** Activity replacement */
#define MSGID_ACTIVITY_NOT_REPLACED                     "ACTIVITY_NOT_REPLACED" /** Activity not replaced */
#define MSGID_LOAD_ACTIVITIES_FROM_DB_FAIL              "LOAD_ACTIVITIES_FROM_DB_FAIL" /** failed to load activities from Db */
#define MSGID_RETRIEVE_ID_FAIL                          "RETRIEVE_ID_FAIL" /** Error retrieving _id from results returned from MojoDB */
#define MSGID_ID_NOT_FOUND                              "ID_NOT_FOUND" /** _id not found loading Activities from MojoDB */
#define MSGID_REV_NOT_FOUND                             "REV_NOT_FOUND" /** _rev not found loading Activities from MojoDB */
#define MSGID_CREATE_ACTIVITY_EXCEPTION                 "CREATE_ACTIVITY_EXCEPTION" /** Exception during activity create */
#define MSGID_UNKNOWN_EXCEPTION                         "UNKNOWN_EXCEPTION" /** Unknown exception during activity create */
#define MSGID_ACTIVITY_ID_REG_FAIL                      "ACTIVITY_ID_REG_FAIL" /** Failed to register ID */
#define MSGID_ACTIVITY_NAME_REG_FAIL                    "ACTIVITY_NAME_REG_FAIL" /** Failed to register activity name */
#define MSGID_GET_PAGE_FAIL                             "GET_PAGE_FAIL" /** Error getting page parameter in MojoDB query response */
#define MSGID_ACTIVITIES_PURGE_FAILED                   "ACTIVITIES_PURGE_FAILED" /** Purge of batch of old Activities failed */
#define MSGID_RETRY_ACTIVITY_PURGE                      "RETRY_ACTIVITY_PURGE" /** retrying purge of batch of old Activities */
#define MSGID_ACTIVITY_CONFIG_LOAD_FAIL                 "ACTIVITY_CONFIG_LOAD_FAIL" /** Failed to load static Activity configuration */
#define MSGID_OLD_ACTIVITY_NO_REPLACE                   "OLD_ACTIVITY_NO_REPLACE" /** activity already exists and in new activity replace was not specified*/
#define MSGID_STOP_ACTIVITY_REQ_REPLY_FAIL              "STOP_ACTIVITY_REQ_REPLY_FAIL" /** Failed to generate reply to Stop activity request */
#define MSGID_CANCEL_ACTIVITY_REQ_REPLY_FAIL            "CANCEL_ACTIVITY_REQ_REPLY_FAIL" /** Failed to generate reply to Cancel activity request */
#define MSGID_SERIAL_NUM_MISMATCH                       "SERIAL_NUM_MISMATCH" /** call sequence serial numbers do not match */
#define MSGID_NO_OLD_ACTIVITY                           "NO_OLD_ACTIVITY" /** Old activity to be replaced not specified */
#define MSGID_NEW_ACTIVITY_PERSIST_TOKEN_SET            "NEW_ACTIVITY_PERSIST_TOKEN_SET" /** Persist token set in new activity which is to replace an existing old activity */
#define MSGID_SUBSCRIBER_NOT_FOUND                      "SUBSCRIBER_NOT_FOUND" /** subscriber not found on subscribers list */
#define MSGID_ADD_REQ_OWNER_MISMATCH                    "ADD_REQ_OWNER_MISMATCH" /** addition of requirements from one activity to another activity */
#define MSGID_LINKED_REQUIREMENT_FOUND                  "LINKED_REQUIREMENT_FOUND" /** Found linked requirement adding requirement to activity */
#define MSGID_REMOVE_REQ_OWNER_MISMATCH                 "REMOVE_REQ_OWNER_MISMATCH" /** Requirement owner mismatch */
#define MSGID_UNOWNED_UPDATED_REQUIREMENT               "UNOWNED_UPDATED_REQUIREMENT" /** Updated requirement is not owned by activity */
#define MSGID_SAME_ACTIVITY_ID_FOUND                    "SAME_ACTIVITY_ID_FOUND" /** Found existing Activity with same id */
#define MSGID_ACTIVITY_NOT_FOUND                        "ACTIVITY_NOT_FOUND" /** activity not found in Activity table while attempting to release */
#define MSGID_UNFOCUS_ACTIVITY_FAILED                   "UNFOCUS_ACTIVITY_FAILED" /** Can't remove focus from activity which is not focused */
#define MSGID_ACTIVITY_NOT_ON_FOCUSSED_LIST             "ACTIVITY_NOT_ON_FOCUSSED_LIST" /** activity not on focus list while removing focus */
#define MSGID_SRC_ACTIVITY_UNFOCUSSED                   "SRC_ACTIVITY_UNFOCUSSED" /** Can't add focus from src_activity to target_activity as source is not focused */
#define MSGID_ACTIVITY_NOT_ON_BACKGRND_Q                "ACTIVITY_NOT_ON_BACKGRND_Q" /** activity not found on background queue */
#define MSGID_ACTIVITY_NOT_ON_READY_Q                   "ACTIVITY_NOT_ON_READY_Q" /**activity not found on ready queue */
#define MSGID_ATTEMPT_RUN_BACKGRND_ACTIVITY             "ATTEMPT_RUN_BACKGRND_ACTIVITY" /** activity was not queued attempting to run background Activity */
#define MSGID_ACTIVITY_ALREADY_ASSOCIATD                "ACTIVITY_ALREADY_ASSOCIATD" /** acivity already associated with entity */
#define MSGID_ACTIVITY_DISSOCIATD                       "ACTIVITY_DISSOCIATD" /** Activity currently not associated */
#define MSGID_ACTIVITY_ENTITY_UNLINKED                  "ACTIVITY_ENTITY_UNLINKED" /** Association between activity and entity was not linked */
#define MSGID_ASSOSCIATN_UPDATE_FAIL                    "ASSOSCIATN_UPDATE_FAIL" /** Unable to update association between activity and entity as another association is already present */
#define MSGID_REQUIREMENT_INSTANTIATE_FAIL              "REQUIREMENT_INSTANTIATE_FAIL" /** manager does not know how to instantiate requirement for activity */
#define MSGID_UNSOLVABLE_CONN_MGR_SUBSCR_ERR            "UNSOLVABLE_CONN_MGR_SUBSCR_ERR" /** Subscription to Connection Manager experienced an uncorrectable failure */
#define MSGID_CONN_MGR_SUBSCR_ERR                       "CONN_MGR_SUBSCR_ERR" /** Subscription to Connection Manager failed */
#define MSGID_WIFI_CONN_STATUS_UNKNOWN                  "WIFI_CONN_STATUS_UNKNOWN" /** Wifi connection status not returned by Connection Manager */
#define MSGID_WIFI_STATUS_UNKNOWN                       "WIFI_STATUS_UNKNOWN" /** Wifi status not returned by Connection Manager */
#define MSGID_WAN_CONN_STATUS_UNKNOWN                   "WAN_CONN_STATUS_UNKNOWN" /** WAN connection status not returned by Connection Manager */
#define MSGID_WAN_NW_MODE_UNKNOWN                       "WAN_NW_MODE_UNKNOWN" /** WAN network mode not returned by Connection Manager */
#define MSGID_WAN_STATUS_UNKNOWN                        "WAN_STATUS_UNKNOWN" /** WAN status not returned by Connection Manager */
#define MSGID_GET_NW_CONFIDENCE_FAIL                    "GET_NW_CONFIDENCE_FAIL" /** Failed to retreive network confidence from network description */
#define MSGID_NON_STRING_TYPE_NW_CONFIDENCE             "NON_STRING_TYPE_NW_CONFIDENCE" /** Network confidence must be specified as a string */
#define MSGID_GET_NW_CONFIDENCE_LEVEL_FAIL              "GET_NW_CONFIDENCE_LEVEL_FAIL" /** Failed to retreive network confidence level as string */
#define MSGID_UNKNOWN_CONN_CONFIDENCE_STR               "UNKNOWN_CONN_CONFIDENCE_STR" /** Unknown connection confidence string */
#define MSGID_UNKNOWN_CONN_CONFIDENCE_LEVEL             "UNKNOWN_CONN_CONFIDENCE_LEVEL" /** Unknown connection confidence level during update confidence requirements */
#define MSGID_CONTAINER_PRIORITY_CHANGE_FAIL            "CONTAINER_PRIORITY_CHANGE_FAIL" /** failed to change priority of the container */
#define MSGID_CONTROL_FILE_OPEN_FAIL                    "CONTROL_FILE_OPEN_FAIL" /** Failed to open control file */
#define MSGID_INEXISTENT_PROCESS_WRITTEN                "INEXISTENT_PROCESS_WRITTEN" /** Process to be written to file doesn't exist */
#define MSGID_WRITE_TO_CONTROL_FILE_FAIL                "WRITE_TO_CONTROL_FILE_FAIL" /** Failed to write control file */
#define MSGID_SHORT_WRITE_UPDATE                        "SHORT_WRITE_UPDATE" /** Short write updating control file */
#define MSGID_CGROUP_FS_ROOT_STAT_FAIL                  "CGROUP_FS_ROOT_STAT_FAIL" /** Error attempting to stat cgroup filesystem root */
#define MSGID_FS_TYPE_MISMATCH                          "FS_TYPE_MISMATCH" /** cgroup filesystem root type does not match cgroup magic type */
#define MSGID_UNSOLVABLE_LUNABUS_SUBSCR_FAIL            "UNSOLVABLE_LUNABUS_SUBSCR_FAIL"/** Subscription to Luna Bus updates experienced an uncorrectable failure */
#define MSGID_LUNABUS_SUBSCR_FAIL                       "LUNABUS_SUBSCR_FAIL" /** Subscription to Luna Bus updates failed */
#define MSGID_UNFORMATTED_LUNABUS_UPDATE                "UNFORMATTED_LUNABUS_UPDATE" /** badly formatted Luna bus update, services is not an array */
#define MSGID_GET_SRVC_NAME_FAIL                        "GET_SRVC_NAME_FAIL" /** Failed to retreive service name while processing batch service status update---->2 */
#define MSGID_GET_ALL_NAMES_FAIL                        "GET_ALL_NAMES_FAIL" /** Failed to retreive equivalent service names for service while processing batch service status update --->2*/
#define MSGID_GET_PID_FAIL                              "GET_PID_FAIL" /** Failed to retreive process id for the service while processing batch service status update --->2*/
#define MSGID_VALID_SRVC_NAME_NOT_FOUND                 "VALID_SRVC_NAME_NOT_FOUND" /** No valid names found for service while processing batch service status update */
#define MSGID_GET_CONNECTED_STATE_FAIL                  "GET_CONNECTED_STATE_FAIL" /** Failed to retreive connected state processing service update from Luna bus hub */
#define MSGID_POPULATE_BUSID_FAIL                       "POPULATE_BUSID_FAIL" /** Bus names not passed as an array of ids */
#define MSGID_GET_BUSID_FAIL                            "GET_BUSID_FAIL" /** Failed to retreive bus id from JSON object */
#define MSGID_CALL_RESP_UNHANDLED_EXCEPTION             "CALL_RESP_UNHANDLED_EXCEPTION" /** Unhandled exception occurred processing response */
#define MSGID_CALL_RESP_UNKNOWN_EXCEPTION               "CALL_RESP_UNKNOWN_EXCEPTION" /** Unhandled exception of unknown type occurred processing response */
#define MSGID_NON_LUNA_BUS_MSG                          "NON_LUNA_BUS_MSG" /** Message did not originate from the Luna Bus--->2 */
#define MSGID_UNHANDLED_RESP                            "UNHANDLED_RESP" /** Unhandled response */
#define MSGID_PERSIST_CMD_VALIDATE_EXCEPTION            "PERSIST_CMD_VALIDATE_EXCEPTION" /** unexpected exception during command calidation */
#define MSGID_PERSIST_CMD_NO_RESULTS                    "PERSIST_CMD_NO_RESULTS" /** Results of MojoDB persist command not found in response */
#define MSGID_PERSIST_CMD_EMPTY_RESULTS                 "PERSIST_CMD_EMPTY_RESULTS" /** MojoDB persist command returned empty result set */
#define MSGID_PERSIST_CMD_GET_ID_ERR                    "PERSIST_CMD_GET_ID_ERR" /** Error retreiving _id from MojoDB persist command response */
#define MSGID_PERSIST_CMD_NO_ID                         "PERSIST_CMD_NO_ID" /** _id not found in MojoDB persist command response */
#define MSGID_PERSIST_CMD_RESP_REV_NOT_FOUND            "PERSIST_CMD_RESP_REV_NOT_FOUND" /** _rev not found in MojoDB persist command response */
#define MSGID_PERSIST_TOKEN_VAL_UPDATE_FAIL             "PERSIST_TOKEN_VAL_UPDATE_FAIL" /** Failed to set or update value of persist token */
#define MSGID_DECODE_CREATOR_ID_FAILED                  "DECODE_CREATOR_ID_FAILED" /** Unable to decode creator id while reloading*/
#define MSGID_PBUS_CALLER_SETTING_CREATOR               "PBUS_CALLER_SETTING_CREATOR" /** Attempt to set creator by caller from the public bus */
#define MSGID_EXCEPTION_IN_RELOADING_ACTIVITY           "EXCEPTION_IN_RELOADING_ACTIVITY" /** Unexpected exception reloading Activity */
#define MSGID_RELOAD_ACTVTY_UNKNWN_EXCPTN               "RELOAD_ACTVTY_UNKNWN_EXCPTN" /** Unknown exception reloading Activity */
#define MSGID_RM_ACTVTY_CB_ATTEMPT                      "RM_ACTVTY_CB_ATTEMPT" /** Attempt to remove callback on Complete */
#define MSGID_DECODE_METADATA_FAIL                      "DECODE_METADATA_FAIL" /** Failed to decode metadata to JSON while attempting to update */
#define MSGID_END_TIME_FORMAT_ERR                       "END_TIME_FORMAT_ERR" /** Last finished should use the same time format as the other times in the schedule */
#define MSGID_MGR_NOT_FOUND_FOR_REQUIREMENT             "MGR_NOT_FOUND_FOR_REQUIREMENT" /** Unable to find Manager for requirement */
#define MSGID_PERSISTED_ANONID_FOUND                    "PERSISTED_ANONID_FOUND" /** anonId subscriber found persisted in MojoDB */
#define MSGID_PERSIST_ATMPT_UNEXPECTD_EXCPTN            "PERSIST_ATMPT_UNEXPECTD_EXCPTN" /** Unexpected exception while attempting to persist */
#define MSGID_PERSIST_ATMPT_UNKNWN_EXCPTN               "PERSIST_ATMPT_UNKNWN_EXCPTN" /** Unknown exception while attempting to persist */
#define MSGID_PERSIST_CMD_RESP_FAIL                     "PERSIST_CMD_RESP_FAIL" /** Mojo persist command failed */
#define MSGID_PERSIST_CMD_TRANSIENT_ERR                 "PERSIST_CMD_TRANSIENT_ERR" /** Mojo persist command failed with transient error */
#define MSGID_UNHOOK_CMD_ACTVTY_ERR                     "UNHOOK_CMD_ACTVTY_ERR" /** Attempt to unhook PersistCommand assigned to different activity */
#define MSGID_UNHOOK_CMD_QUEUE_ORDERING_ERR             "UNHOOK_CMD_QUEUE_ORDERING_ERR" /** Request to unhook persistCommand which is not the first persist command in the queue */
#define MSGID_HOOKED_PERSIST_CMD_NOT_FOUND              "HOOKED_PERSIST_CMD_NOT_FOUND" /** Attempt to retreive the current hooked persist command, but none is present */
#define MSGID_FINISH_ACTVTY_REPLY_ERR                   "FINISH_ACTVTY_REPLY_ERR" /** Failed to generate reply to Complete request */
#define MSGID_UNHOOK_CMD_NOT_IN_QUEUE                   "UNHOOK_CMD_NOT_IN_QUEUE" /** Request to unhook persistCommand which is not in the queue */

/** Timeout.cpp */
#define MSGID_TIMEOUT_EXCEPTION                         "TIMEOUT_EXCEPTION"  /* */
#define MSGID_TIMEOUT_ERR_UNKNOWN                       "TIMEOUT_ERR_UNKNOWN"  /* */


/** TestCategory.cpp */
#define MSGID_TEST_LEAK_ACTIVITY                        "TEST_LEAK_ACTIVITY"  /* */
#define MSGID_TEST_RELEASE_ACTIVITY                     "TEST_RELEASE_ACTIVITY"  /* */

/** TelephonyProxy.cpp */
#define MSGID_TIL_UNKNOWN_REQ                  "TIL_UNKNOWN_REQ"  /* does not know how to instantiate Requirement */
#define MSGID_TIL_NWSTATUS_QUERY_ERR           "TIL_NWSTATUS_QUERY_ERR"  /* TIL experienced an uncorrectable failure */
#define MSGID_TIL_NWSTATUS_QUERY_RETRY         "TIL_NWSTATUS_QUERY_RETRY"  /* Subscription to TIL failed, retrying */
#define MSGID_TIL_NWSTATE_ERR                  "TIL_NWSTATE_ERR"  /* Error attempting to retrieve network state */
#define MSGID_TIL_NO_NWSTATE                   "TIL_NO_NWSTATE"  /* Network status update message did not include network state */
#define MSGID_TIL_QUERYUPDATE_ERR              "TIL_QUERYUPDATE_ERR"  /* Platform query subscription to TIL experienced an uncorrectable failure */
#define MSGID_TIL_QUERYUPDATE_RETRY            "TIL_QUERYUPDATE_RETRY"  /* Platform query subscription to TIL failed, retrying */
#define MSGID_TIL_NO_EXTENDED                  "TIL_NO_EXTENDED" /* Platform query update did not contain extended */
#define MSGID_TIL_NO_CAPABILITIES              "TIL_NO_CAPABILITIES" /* Platform query update did not contain capabilities */
#define MSGID_TIL_NO_MAPCE_ENABLE              "TIL_NO_MAPCE_ENABLE" /* Platform query update message did not include mapcenable */

/** SystemManagerProxy.cpp */
#define MSGID_SM_UNKNOWN_REQ                   "SM_UNKNOWN_REQ" /* system manager does not know how to instantiate Requirement */
#define MSGID_SM_BOOTSTS_UPDATE_FAIL           "SM_BOOTSTS_UPDATE_FAIL"  /*System Manager experienced an uncorrectable failure */
#define MSGID_SM_BOOTSTS_UPDATE_RETRY          "SM_BOOTSTS_UPDATE_RETRY"  /*Subscription to System Manager failed, retrying */
#define MSGID_SM_BOOTSTS_NOTRETURNED           "SM_BOOTSTS_NOTRETURNED"  /* Bootup status not returned by System Manager */

/** Subscription.cpp **/
#define MSGID_SUBSCRIPTION_CANCEL_FAIL          "SUBSCRIPTION_CANCEL_FAIL"  /* Unhandled exception occurred while cancelling subscription */
#define MSGID_SUBSCRIPTION_CANCEL_ERR           "SUBSCRIPTION_CANCEL_ERR"  /* Unhandled exception of unknown type occurred cancelling subscription*/

/** ServiceApp.cpp */
#define MSGID_UPSTART_EMIT_FAIL                  "UPSTART_EMIT_FAIL"  /* ServiceApp: Failed to emit upstart event */
#define MSGID_UPSTART_EMIT_ALLOC_FAIL            "UPSTART_EMIT_ALLOC_FAIL"  /* ServiceApp: Failed to allocate memory for upstart emit*/
#define MSGID_INIT_RNG_FAIL                      "INIT_RNG_FAIL"  /* Failed to initialize the RNG state */

/** Scheduler.cpp */
#define MSGID_SCHE_OFFSET_NOTSET                  "SCHE_OFFSET_NOTSET"  /* Attempt to access local offset before it has been set */

/** ResourceManager.cpp */
#define MSGID_RM_ALREADY_SET                  "RM_ALREADY_SET"  /* Resource Manager for resource has already been set*/

/** RequirementManager.cpp */
#define MSGID_REQ_MANAGER_NOT_FOUND          "REQ_MANAGER_NOT_FOUND" /* No manager found for Requirement */

/** PowerdScheduler.cpp */
#define MSGID_SET_TIMEOUT_PARAM_ERR          "SET_TIMEOUT_PARAM_ERR" /* Error constructing parameters for powerd set timeout call*/
#define MSGID_CLEAR_TIMEOUT_PARAM_ERR        "CLEAR_TIMEOUT_PARAM_ERR" /* Error constructing parameters for powerd clear timeout call*/
#define MSGID_GET_SYSTIME_PARAM_ERR          "GET_SYSTIME_PARAM_ERR" /* Error constructing parameters for subscription to System Time Service */
#define MSGID_SCH_WAKEUP_REG_ERR             "SCH_WAKEUP_REG_ERR" /* Failed to register scheduled wakeup */
#define MSGID_SCH_WAKEUP_REG_RETRY           "SCH_WAKEUP_REG_RETRY" /* Failed to register scheduled wakeup , retry */
#define MSGID_SCH_WAKEUP_CANCEL_ERR          "SCH_WAKEUP_CANCEL_ERR" /* Failed to cancel scheduled wakeup */
#define MSGID_SCH_WAKEUP_CANCEL_RETRY        "SCH_WAKEUP_CANCEL_RETRY" /* Failed to cancel scheduled wakeup , retry */
#define MSGID_SYS_TIME_RSP_FAIL              "SYS_TIME_RSP_FAIL" /* System Time subscription experienced an uncorrectable failure */
#define MSGID_GET_SYSTIME_RETRY              "GET_SYSTIME_RETRY" /* System Time subscription failed, retry */
#define MSGID_SYSTIME_NO_OFFSET              "SYSTIME_NO_OFFSET" /* ystem Time message is missing timezone offset */


/** PowerdProxy.cpp */
#define MSGID_CHARGER_STATUS_ERR              "CHARGER_STATUS_ERR" /* Failed to trigger charger status signal */
#define MSGID_BATTERY_STATUS_ERR              "BATTERY_STATUS_ERR" /* Failed to trigger battery status signal */
#define MSGID_CHRGR_STATUS_SIG_FAIL           "CHRGR_STATUS_SIG_FAIL" /* Subscription to charger status signal experienced an uncorrectable failure */
#define MSGID_CHRGR_SIG_ENABLE                "CHRGR_SIG_ENABLE" /* Subscription to charger status signal failed, resubscribing */
#define MSGID_CHRGR_SIG_NO_TYPE               "CHRGR_SIG_NO_TYPE" /* Error retrieving charger type from charger status signal response */
#define MSGID_CHRGR_TYPE_UNKNOWN              "CHRGR_TYPE_UNKNOWN" /* Unknown charger type */
#define MSGID_CHRGR_NOT_CONNECTED             "CHRGR_NOT_CONNECTED" /* charger type found , but no connected */
#define MSGID_CHRGR_NO_NAME                   "CHRGR_NO_NAME" /* charger type found , but no name */
#define MSGID_BATTERY_STATUS_SIG_FAIL         "BATTERY_STATUS_SIG_FAIL" /* Subscription to battery status signal experienced an uncorrectable failure */
#define MSGID_BATTERY_SIG_ENABLE              "BATTERY_SIG_ENABLE" /* Subscription to battery status signal failed, resubscribing */
#define MSGID_UNKNOW_REQUIREMENT              "UNKNOW_REQUIREMENT"  /* Attempt to instantiate unknown requirement */

/** PowerdPowerActivity.cpp */
#define MSGID_PWR_LOCK_CREATE_FAIL            "PWR_LOCK_CREATE_FAIL" /* Failed to issue command to create power lock */
#define MSGID_PWR_LOCK_FAKE_NOTI              "PWR_LOCK_FAKE_NOTI" /* Faking power locked notification */
#define MSGID_PWR_UNLOCK_CREATE_FAIL          "PWR_LOCK_CREATE_FAIL" /* Failed to issue command to create power unlock */
#define MSGID_PWR_UNLOCK_FAKE_NOTI            "PWR_LOCK_FAKE_NOTI" /* Faking power unlocked notification */
#define MSGID_PWRLK_NOTI_CREATE_FAIL          "PWRLK_NOTI_CREATE_FAIL" /* Attempt to create power lock failed, retrying. */
#define MSGID_PWRLK_NOTI_UPDATE_FAIL          "PWRLK_NOTI_UPDATE_FAIL" /* Attempt to update power lock failed, retrying.*/
#define MSGID_PWRULK_NOTI_ERR                 "PWRULK_NOTI_ERR" /* Attempt to power unlock failed, retrying.*/
#define MSGID_PWR_UNLOCK_CREATE_ERR           "PWR_UNLOCK_CREATE_ERR" /* Failed to issue command to power lock in unlock noti*/
#define MSGID_PWR_ACTIVITY_CREATE_ERR         "PWR_ACTIVITY_CREATE_ERR" /* Failed to issue command to power lock in timeout */
#define MSGID_PWR_TIMEOUT_NOTI                "PWR_TIMEOUT_NOTI" /* Failed to issue command to power lock in timeout */

/** Activity.cpp */
#define MSGID_ACTIVITY_CB_FAIL                  "ACTIVITY_CB_FAIL" /* Activity Call back failed */
#define MSGID_FOUND_NEW_PARENT                  "FOUND_NEW_PARENT" /* Removing ending flag since found new parent */
#define MSGID_ACTIVITY_TYPE_ERR                 "ACTIVITY_TYPE_ERR" /* immediate and priority were individually set,and will be individually output */
#define MSGID_HOOK_PERSIST_CMD_ERR              "HOOK_PERSIST_CMD_ERR" /* Attempt to hook PersistCommand */
#define MSGID_REQ_UNMET_OWNER_MISMATCH          "REQ_UNMET_OWNER_MISMATCH" /* Requirement owner mismatch */
#define MSGID_REQ_MET_OWNER_MISMATCH            "REQ_MET_OWNER_MISMATCH" /* Requirement owner mismatch */

#define MSGID_CANT_TRIGGER_DIFF_ACTIVITY        "CANT_TRIGGER_DIFF_ACTVTY" /** Can't arm trigger for a different Activity */
#define MSGID_DISARM_TRIGGER_FOR_ACTIVITY       "DISARM_TRIGGER_FOR_ACTVTY" /** Can't disarm trigger for a different Activity */
#define MSGID_CHECK_TRIGGER_FIRED               "CHECK_TRIGGER_FIRED" /** Checking to see if a trigger has fired it does not own */
#define MSGID_METADATA_UPDATE_TO_NONOBJ_TYPE    "METADATA_UPDATE_TO_NONOBJ_TYPE" /** Attempt to update metadata to non-object type */

#define MSGID_APPEND_CMD_TOSELF              "APPEND_CMD_TOSELF" /** Attempt to append Persist Command directly to itself */
#define MSGID_APPEND_CREATE_LOOP             "APPEND_CREATE_LOOP" /**  Attempt to append Persist Command would create a loop */
#define MSGID_CMD_COMPLETE_EXCEPTION         "CMD_COMPLETE_EXCEPTION" /** Unexpected exception %s while trying to complete */
#define MSGID_CMD_UNHOOK_EXCEPTION           "CMD_UNHOOK_EXCEPTION"  /** Unexpected exception unhooking command */ 
#define MSGID_PERSIST_TOKEN_NOT_SET         "PERSIST_TOKEN_NOT_SET"   /** Persist token for Activity is not set */
#define MSGID_PERSIST_TOKEN_INVALID         "PERSIST_TOKEN_INVALID"   /** Persist token for Activity is but not valid */

#define MSGID_RELEASE_ACTIVITY_NOTFOUND     "REL_ACTIVITY_NOTFOUND"  /* Not found in Activity table while attempting to release*/
#define MSGID_RM_ASSOCIATION_NOT_FOUND      "RM_ASSOCIATION_NOTFOUND" /* can't remove association with Entity */

#define MSGID_RM_REQ_NOT_FOUND              "RM_REQ_NOT_FOUND"  /* not found while trying to remove by name */
/** list of logkey ID's */


extern PmLogContext getactivitymanagercontext();

#endif // __ACTIVITYMANAGER_LOGGING_H__
