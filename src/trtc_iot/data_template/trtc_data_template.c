/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "qcloud_iot_trtc_data_tempalte.h"

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "utils_list.h"
#include "TencentSDKWarpper.h"

#define TOTAL_TRTC_SYS_PROPERTY_COUNTS (3)
#define TOTAL_TRTC_SYS_ACTION_COUNTS   (1)
#define TRTC_ABORT_TIMEOUT             15

#define VIDEO_TEST_FILE_PATH "./ruguo-640x360.mp4"
#define AUDIO_TEST_FILE_PATH "./ruguo-640x360.mp4"

static sDataPoint sg_trtc_data[TOTAL_TRTC_SYS_PROPERTY_COUNTS];
static char       sg_data_report_buffer[2048];
static size_t     sg_data_report_buffersize = sizeof(sg_data_report_buffer) / sizeof(sg_data_report_buffer[0]);

typedef struct _TRTC_SYS_PROPERTY {
    TYPE_DEF_TEMPLATE_ENUM   m_sys_video_call_status;
    TYPE_DEF_TEMPLATE_ENUM   m_sys_audio_call_status;
    TYPE_DEF_TEMPLATE_STRING m_sys_userid[2048 + 1];
} _TRTC_SYS_PROPERTY;

static _TRTC_SYS_PROPERTY sg_trtc_property;

static void _init_trtc_property(void)
{
    sg_trtc_property.m_sys_video_call_status = 0;
    sg_trtc_data[0].data_property.data       = &sg_trtc_property.m_sys_video_call_status;
    sg_trtc_data[0].data_property.key        = "_sys_video_call_status";
    sg_trtc_data[0].data_property.type       = TYPE_TEMPLATE_ENUM;
    sg_trtc_data[0].state                    = eCHANGED;

    sg_trtc_property.m_sys_audio_call_status = 0;
    sg_trtc_data[1].data_property.data       = &sg_trtc_property.m_sys_audio_call_status;
    sg_trtc_data[1].data_property.key        = "_sys_audio_call_status";
    sg_trtc_data[1].data_property.type       = TYPE_TEMPLATE_ENUM;
    sg_trtc_data[1].state                    = eCHANGED;

    sg_trtc_property.m_sys_userid[0]   = '\0';
    sg_trtc_data[2].data_property.data = sg_trtc_property.m_sys_userid;
    sg_trtc_data[2].data_property.data_buff_len =
        sizeof(sg_trtc_property.m_sys_userid) / sizeof(sg_trtc_property.m_sys_userid[2]);
    sg_trtc_data[2].data_property.key  = "_sys_userid";
    sg_trtc_data[2].data_property.type = TYPE_TEMPLATE_STRING;
    sg_trtc_data[2].state              = eCHANGED;
};

static TYPE_DEF_TEMPLATE_INT    sg_sys_trtc_join_room_in_SdkAppId                = 0;
static TYPE_DEF_TEMPLATE_STRING sg_sys_trtc_join_room_in_UserId[128 + 1]         = {0};
static TYPE_DEF_TEMPLATE_STRING sg_sys_trtc_join_room_in_UserSig[256 + 1]        = {0};
static TYPE_DEF_TEMPLATE_STRING sg_sys_trtc_join_room_in_StrRoomId[2048 + 1]     = {0};
static TYPE_DEF_TEMPLATE_STRING sg_sys_trtc_join_room_in_PrivateMapKey[2048 + 1] = {0};
static DeviceProperty           sg_sys_trtc_action_in_join_room[]                = {

    {.key = "SdkAppId", .data = &sg_sys_trtc_join_room_in_SdkAppId, .type = TYPE_TEMPLATE_INT},
    {.key = "UserId", .data = sg_sys_trtc_join_room_in_UserId, .type = TYPE_TEMPLATE_STRING},
    {.key = "UserSig", .data = sg_sys_trtc_join_room_in_UserSig, .type = TYPE_TEMPLATE_STRING},
    {.key = "StrRoomId", .data = sg_sys_trtc_join_room_in_StrRoomId, .type = TYPE_TEMPLATE_STRING},
    {.key = "PrivateMapKey", .data = sg_sys_trtc_join_room_in_PrivateMapKey, .type = TYPE_TEMPLATE_STRING},
};
static TYPE_DEF_TEMPLATE_INT sg_sys_trtc_join_room_out_Code     = 0;
static DeviceProperty        sg_sys_trtc_action_out_join_room[] = {
    {.key = "Code", .data = &sg_sys_trtc_join_room_out_Code, .type = TYPE_TEMPLATE_INT},
};

static DeviceAction sg_trtc_actions[] = {
    {
        .pActionId  = "_sys_trtc_join_room",
        .timestamp  = 0,
        .input_num  = sizeof(sg_sys_trtc_action_in_join_room) / sizeof(sg_sys_trtc_action_in_join_room[0]),
        .output_num = sizeof(sg_sys_trtc_action_out_join_room) / sizeof(sg_sys_trtc_action_out_join_room[0]),
        .pInput     = sg_sys_trtc_action_in_join_room,
        .pOutput    = sg_sys_trtc_action_out_join_room,
    },
};

typedef enum { eCALL_IDLE = 0, eCALL_WAITING = 1, eCALLING = 2 } eCallStatus;

typedef struct _TrtcTimer_ {
    Timer        timer;
    int          valid;
    unsigned int timeout;  // unit:s
} TrtcTimer;

typedef struct _TrtcHandle_ {
    int       video_call_status_now;
    int       audio_call_status_now;
    bool      trtc_ctl_msg_arrived;
    void *    mutex;
    List *    trtc_req_list;
    TrtcTimer waiting_timer;
    TrtcTimer calling_timer;
} TrtcHandle;

static void *sg_trtc_client = NULL;

static void _trtc_timer_init(TrtcTimer *trtc_timer, unsigned int timeout, int enable)
{
    if (trtc_timer) {
        HAL_Timer_init(&trtc_timer->timer);
        HAL_Timer_countdown(&trtc_timer->timer, timeout);
        trtc_timer->timeout = timeout;
        trtc_timer->valid   = enable;
    }
}

static bool _trtc_timer_timeout(TrtcTimer *trtc_timer)
{
    bool rc = false;
    if (trtc_timer) {
        if (trtc_timer->valid) {
            rc = HAL_Timer_expired(&trtc_timer->timer);
        }
    }

    return rc;
}

static void _trtc_control_msg_callback(void *pTemplate_client, const char *pJsonValueBuffer, uint32_t valueLength,
                                       DeviceProperty *pProperty)
{
    if (!sg_trtc_client) {
        Log_e("Trtc is not init!");
        return;
    }
    int         val         = 0;
    TrtcHandle *trtc_handle = (TrtcHandle *)sg_trtc_client;

    if (trtc_handle->video_call_status_now == eCALL_IDLE && trtc_handle->audio_call_status_now == eCALL_IDLE) {
        if (strcmp(sg_trtc_data[0].data_property.key, pProperty->key) == 0) {  // video
            val = *(int *)(pProperty->data);
            if (eCALL_WAITING != val) {
                return;
            }

            trtc_handle->video_call_status_now = *(int *)(sg_trtc_data[0].data_property.data) = eCALL_WAITING;
            sg_trtc_data[0].state                                                             = eCHANGED;
            trtc_handle->trtc_ctl_msg_arrived                                                 = true;
            _trtc_timer_init(&trtc_handle->waiting_timer, TRTC_ABORT_TIMEOUT, 1);
        } else if (strcmp(sg_trtc_data[1].data_property.key, pProperty->key) == 0) {  // audio
            val = *(int *)(pProperty->data);
            if (eCALL_WAITING != val) {
                return;
            }

            trtc_handle->audio_call_status_now = *(int *)(sg_trtc_data[1].data_property.data) = eCALL_WAITING;
            sg_trtc_data[1].state                                                             = eCHANGED;
            trtc_handle->trtc_ctl_msg_arrived                                                 = true;
            _trtc_timer_init(&trtc_handle->waiting_timer, TRTC_ABORT_TIMEOUT, 1);
        }
    }
}

static void _trtc_action_callback(void *pClient, const char *pClientToken, DeviceAction *pAction)
{
    int        ret = 0;
    sReplyPara replyPara;
    memset((char *)&replyPara, 0, sizeof(sReplyPara));
    DeviceProperty *pActionInput = pAction->pInput;

    if (!sg_trtc_client) {
        Log_e("Trtc is not init!");
        return;
    }
    TrtcHandle *trtc_handle = (TrtcHandle *)sg_trtc_client;

    IOT_Trtc_Params params;
    params.sdk_app_id  = *(int *)pActionInput[0].data;
    params.user_id     = pActionInput[1].data;
    params.user_sig    = pActionInput[2].data;
    params.str_room_id = pActionInput[3].data;
    params.mode        = IOT_TRTC_None;

    if (trtc_handle->video_call_status_now == eCALL_WAITING) {  // video call
        params.user_data = VIDEO_TEST_FILE_PATH;
        params.av_flag   = 1;
        ret              = qcloud_iot_trtc_wrapper_start(params);
        if (!ret) {
            replyPara.code       = eDEAL_SUCCESS;
            replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
            strcpy(replyPara.status_msg, "action execute success!");
            trtc_handle->video_call_status_now = *(int *)(sg_trtc_data[0].data_property.data) = eCALLING;
            sg_trtc_data[0].state                                                             = eCHANGED;

            strcpy((char *)sg_trtc_data[2].data_property.data, params.user_id);
            sg_trtc_data[2].state = eCHANGED;
            _trtc_timer_init(&trtc_handle->calling_timer, TRTC_ABORT_TIMEOUT, 1);
        } else {
            replyPara.code       = eDEAL_FAIL;
            replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
            strcpy(replyPara.status_msg, "action execute failed!");
            trtc_handle->video_call_status_now = *(int *)(sg_trtc_data[0].data_property.data) = eCALL_IDLE;
            sg_trtc_data[0].state                                                             = eCHANGED;
        }
    } else if (trtc_handle->audio_call_status_now == eCALL_WAITING) {  // audio call
        params.user_data = AUDIO_TEST_FILE_PATH;
        params.av_flag   = 0;
        ret              = qcloud_iot_trtc_wrapper_start(params);
        if (!ret) {
            replyPara.code       = eDEAL_SUCCESS;
            replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
            strcpy(replyPara.status_msg, "action execute success!");
            trtc_handle->audio_call_status_now = *(int *)(sg_trtc_data[1].data_property.data) = eCALLING;
            sg_trtc_data[1].state                                                             = eCHANGED;
            // todo report usrid
            strcpy((char *)sg_trtc_data[2].data_property.data, params.user_id);
            sg_trtc_data[2].state = eCHANGED;
            _trtc_timer_init(&trtc_handle->calling_timer, TRTC_ABORT_TIMEOUT, 1);
        } else {
            replyPara.code       = eDEAL_FAIL;
            replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
            strcpy(replyPara.status_msg, "action execute failed!");
            trtc_handle->audio_call_status_now = *(int *)(sg_trtc_data[1].data_property.data) = eCALL_IDLE;
            sg_trtc_data[1].state                                                             = eCHANGED;
        }
    } else {
        replyPara.code       = eDEAL_FAIL;
        replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
        strcpy(replyPara.status_msg, "Invalid status!");
    }

    for (int i = 0; i < pAction->input_num; i++) {
        if (JSTRING == pActionInput[i].type) {
            HAL_Free(pActionInput[i].data);
        }
    }

    // construct output
    DeviceProperty *pActionOutput   = pAction->pOutput;
    *(int *)(pActionOutput[0].data) = replyPara.code;
    IOT_Action_Reply(pClient, pClientToken, sg_data_report_buffer, sg_data_report_buffersize, pAction, &replyPara);
}

static int _register_trtc_property(void *pTemplate_client)
{
    int i, rc;

    _init_trtc_property();
    for (i = 0; i < TOTAL_TRTC_SYS_PROPERTY_COUNTS; i++) {
        rc = IOT_Template_Register_Property(pTemplate_client, &sg_trtc_data[i].data_property,
                                            _trtc_control_msg_callback);
        if (rc != QCLOUD_RET_SUCCESS) {
            return rc;
        }
    }

    return QCLOUD_RET_SUCCESS;
}

static int _register_trtc_action(void *pTemplate_client)
{
    int i, rc;

    for (i = 0; i < TOTAL_TRTC_SYS_ACTION_COUNTS; i++) {
        rc = IOT_Template_Register_Action(pTemplate_client, &sg_trtc_actions[i], _trtc_action_callback);
        if (rc != QCLOUD_RET_SUCCESS) {
            rc = IOT_Template_Destroy(pTemplate_client);
            Log_e("register device data template action failed, err: %d", rc);
            return rc;
        } else {
            Log_i("data template action=%s registered.", sg_trtc_actions[i].pActionId);
        }
    }

    return QCLOUD_RET_SUCCESS;
}

static void _trtc_report_reply_callback(void *pClient, Method method, ReplyAck replyAck, const char *pJsonDocument,
                                        void *pUserdata)
{
    Log_i("recv report reply response, reply ack: %d", replyAck);
}

static int _deal_up_stream_user_logic(DeviceProperty *pReportDataList[], int *pCount)
{
    int i, j;
    for (i = 0, j = 0; i < TOTAL_TRTC_SYS_PROPERTY_COUNTS; i++) {
        if (eCHANGED == sg_trtc_data[i].state) {
            pReportDataList[j++]  = &(sg_trtc_data[i].data_property);
            sg_trtc_data[i].state = eNOCHANGE;
        }
    }
    *pCount = j;
    return (*pCount > 0) ? QCLOUD_RET_SUCCESS : QCLOUD_ERR_FAILURE;
}

void _trtc_reset_status1(void)
{
    TrtcHandle *trtc_handle = (TrtcHandle *)sg_trtc_client;

    *(int *)(sg_trtc_data[0].data_property.data) = eCALL_IDLE;
    *(int *)(sg_trtc_data[1].data_property.data) = eCALL_IDLE;
    sg_trtc_data[0].state                        = eCHANGED;
    sg_trtc_data[1].state                        = eCHANGED;
    if (trtc_handle) {
        _trtc_timer_init(&trtc_handle->waiting_timer, 0, 0);
        _trtc_timer_init(&trtc_handle->calling_timer, 0, 0);
    }
    printf("clear iot device status!\n");
}

void _trtc_reset_status2(void)
{
    TrtcHandle *trtc_handle = (TrtcHandle *)sg_trtc_client;

    if (trtc_handle) {
        trtc_handle->audio_call_status_now = eCALL_IDLE;
        trtc_handle->video_call_status_now = eCALL_IDLE;
    }
}

static void _trtc_abort_timeout(void)
{
    bool        rc          = false;
    TrtcHandle *trtc_handle = (TrtcHandle *)sg_trtc_client;
    if (!trtc_handle) {
        return;
    }

    if ((trtc_handle->video_call_status_now == eCALL_WAITING) ||
        (trtc_handle->audio_call_status_now == eCALL_WAITING)) {
        rc = _trtc_timer_timeout(&trtc_handle->waiting_timer);
    }

    if (rc) {
        _trtc_reset_status1();
        qcloud_iot_trtc_wrapper_stop();
        _trtc_reset_status2();
    }

    rc = false;
    if ((trtc_handle->video_call_status_now == eCALLING) || (trtc_handle->audio_call_status_now == eCALLING)) {
        if (qcloud_iot_trtc_wrapper_usr_state()) {
            _trtc_timer_init(&trtc_handle->calling_timer, 0, 1);
        } else {
            rc = _trtc_timer_timeout(&trtc_handle->calling_timer);
        }
    }

    if (rc && (!qcloud_iot_trtc_wrapper_usr_state())) {
        _trtc_reset_status1();
        qcloud_iot_trtc_wrapper_stop();
        _trtc_reset_status2();
    }
}

/*export api**/
void qcloud_iot_trtc_process(void *pTemplate_client)
{
    int             rc = 0;
    DeviceProperty *pReportDataList[TOTAL_TRTC_SYS_PROPERTY_COUNTS];
    int             ReportCont;
    sReplyPara      replyPara;
    TrtcHandle *    trtc_handle = (TrtcHandle *)sg_trtc_client;
    if (!trtc_handle) {
        return;
    }

    if (trtc_handle->trtc_ctl_msg_arrived) {
        memset(&replyPara, 0, sizeof(sReplyPara));
        replyPara.code          = eDEAL_SUCCESS;
        replyPara.timeout_ms    = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
        replyPara.status_msg[0] = '\0';
        rc = IOT_Template_ControlReply(pTemplate_client, sg_data_report_buffer, sg_data_report_buffersize, &replyPara);
        if (rc == QCLOUD_RET_SUCCESS) {
            Log_d("Contol msg reply success");
            trtc_handle->trtc_ctl_msg_arrived = false;
        } else {
            Log_e("Contol msg reply failed, err: %d", rc);
        }
    }

    if (QCLOUD_RET_SUCCESS == _deal_up_stream_user_logic(pReportDataList, &ReportCont)) {
        rc = IOT_Template_JSON_ConstructReportArray(pTemplate_client, sg_data_report_buffer, sg_data_report_buffersize,
                                                    ReportCont, pReportDataList);
        if (rc == QCLOUD_RET_SUCCESS) {
            rc = IOT_Template_Report(pTemplate_client, sg_data_report_buffer, sg_data_report_buffersize,
                                     _trtc_report_reply_callback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
            if (rc == QCLOUD_RET_SUCCESS) {
                Log_i("data template report success");
            } else {
                Log_e("data template report failed, err: %d", rc);
            }
        } else {
            Log_e("construct report data failed, err: %d", rc);
        }
    }

    _trtc_abort_timeout();
}

void *qcloud_iot_trtc_init(void *pTemplate_client)
{
    TrtcHandle *trtc_handle = NULL;
    int         rc          = QCLOUD_RET_SUCCESS;

    qcloud_iot_trtc_wrapper_init();

    rc = _register_trtc_property(pTemplate_client);
    if (QCLOUD_RET_SUCCESS != rc) {
        Log_e("register trtc system property fail,rc:%d", rc);
        goto exit;
    }

    rc = _register_trtc_action(pTemplate_client);
    if (QCLOUD_RET_SUCCESS != rc) {
        Log_e("register trtc system action fail,rc:%d", rc);
        goto exit;
    }

    trtc_handle = HAL_Malloc(sizeof(TrtcHandle));
    if (!trtc_handle) {
        Log_e("allocate trtc client failed");
        rc = QCLOUD_ERR_MALLOC;
        goto exit;
    }
    memset(trtc_handle, 0, sizeof(TrtcHandle));

    trtc_handle->mutex = HAL_MutexCreate();
    if (trtc_handle->mutex == NULL) {
        Log_e("create asr mutex fail");
        rc = QCLOUD_ERR_FAILURE;
        goto exit;
    }

    trtc_handle->trtc_req_list = list_new();
    if (trtc_handle->trtc_req_list) {
        trtc_handle->trtc_req_list->free = HAL_Free;
    } else {
        Log_e("no memory to allocate asr_req_list");
        rc = QCLOUD_ERR_FAILURE;
    }

    trtc_handle->audio_call_status_now = eCALL_IDLE;
    trtc_handle->video_call_status_now = eCALL_IDLE;
    _trtc_timer_init(&trtc_handle->waiting_timer, 0, 0);
    _trtc_timer_init(&trtc_handle->calling_timer, 0, 0);
    sg_trtc_client = trtc_handle;

exit:

    if (rc != QCLOUD_RET_SUCCESS) {
        if (trtc_handle) {
            HAL_Free(trtc_handle);
            if (trtc_handle->mutex) {
                HAL_MutexDestroy(trtc_handle->mutex);
            }
            if (trtc_handle->trtc_req_list) {
                list_destroy(trtc_handle->trtc_req_list);
            }
        }
        trtc_handle    = NULL;
        sg_trtc_client = NULL;
    }

    return trtc_handle;
}

int qcloud_iot_trtc_destroy(void *handle)
{
    TrtcHandle *trtc_handle = (TrtcHandle *)handle;
    int         rc          = QCLOUD_RET_SUCCESS;

    qcloud_iot_trtc_wrapper_stop();

    if (!handle) {
        Log_e("Input param is NULL!");
        return QCLOUD_ERR_INVAL;
    }

    if (trtc_handle->mutex) {
        HAL_MutexDestroy(trtc_handle->mutex);
    }
    if (trtc_handle->trtc_req_list) {
        list_destroy(trtc_handle->trtc_req_list);
    }

    HAL_Free(trtc_handle);
    sg_trtc_client = NULL;
    return rc;
}

int qcloud_iot_trtc_call(CallType type)
{
    TrtcHandle *trtc_handle = (TrtcHandle *)sg_trtc_client;

    if (!trtc_handle) {
        Log_e("trtc is not initial!");
        return -1;
    }

    if ((eCALL_IDLE != trtc_handle->video_call_status_now) || ((eCALL_IDLE != trtc_handle->audio_call_status_now))) {
        Log_e("device is busy!");
        return -1;
    }

    if ((VIDEO_CALL != type) && (AUDIO_CALL != type)) {
        Log_e("invalid type %d!", type);
        return -1;
    }

    if (VIDEO_CALL == type) {
        trtc_handle->video_call_status_now = *(int *)(sg_trtc_data[0].data_property.data) = eCALL_WAITING;
        sg_trtc_data[0].state                                                             = eCHANGED;
        // HAL_Snprintf((char *)sg_trtc_data[2].data_property.data, 2048, "%s/%s", sg_dev_info.product_id,
        //             sg_dev_info.device_name);
        // sg_trtc_data[2].state = eCHANGED;
    } else if (AUDIO_CALL == type) {
        trtc_handle->audio_call_status_now = *(int *)(sg_trtc_data[1].data_property.data) = eCALL_WAITING;
        sg_trtc_data[1].state                                                             = eCHANGED;
        // HAL_Snprintf((char *)sg_trtc_data[2].data_property.data, 2048, "%s/%s", sg_dev_info.product_id,
        //             sg_dev_info.device_name);
        // sg_trtc_data[2].state = eCHANGED;
    }

    _trtc_timer_init(&trtc_handle->waiting_timer, TRTC_ABORT_TIMEOUT, 1);

    return 0;
}

void qcloud_iot_trtc_hang_up(void)
{
    _trtc_reset_status1();
    qcloud_iot_trtc_wrapper_stop();
    _trtc_reset_status2();
}

#ifdef __cplusplus
}
#endif
