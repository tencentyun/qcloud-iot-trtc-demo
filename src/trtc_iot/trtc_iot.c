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

#include "qcloud_iot_trtc.h"

#include <stdio.h>

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "qcloud_iot_trtc_data_tempalte.h"

static pthread_t sg_datatemplate_thread;
static int       sg_datatemplate_task_exit;

static DeviceInfo sg_dev_info;

// event handle
static void _event_handler(void *pClient, void *handle_context, MQTTEventMsg *msg)
{
    uintptr_t packet_id = (uintptr_t)msg->msg;
    switch (msg->event_type) {
        case MQTT_EVENT_UNDEF:
            Log_i("undefined event occur.");
            break;

        case MQTT_EVENT_DISCONNECT:
            Log_i("MQTT disconnect.");
            break;

        case MQTT_EVENT_RECONNECT:
            Log_i("MQTT reconnect.");
            break;

        case MQTT_EVENT_SUBCRIBE_SUCCESS:
            Log_i("subscribe success, packet-id=%u", packet_id);
            break;

        case MQTT_EVENT_SUBCRIBE_TIMEOUT:
            Log_i("subscribe wait ack timeout, packet-id=%u", packet_id);
            break;

        case MQTT_EVENT_SUBCRIBE_NACK:
            Log_i("subscribe nack, packet-id=%u", packet_id);
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            Log_i("publish success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            Log_i("publish timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            Log_i("publish nack, packet-id=%u", (unsigned int)packet_id);
            break;
        default:
            Log_i("Should NOT arrive here.");
            break;
    }
}

// set up MQTT construct parameters
static void _setup_connect_init_params(TemplateInitParams *init_params, DeviceInfo *device_info)
{
    init_params->device_name            = device_info->device_name;
    init_params->product_id             = device_info->product_id;
    init_params->device_secret          = device_info->device_secret;
    init_params->command_timeout        = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    init_params->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
    init_params->auto_connect_enable    = 1;
    init_params->event_handle.h_fp      = _event_handler;
}

static void *_datatemplate_task_entry(void *args)
{
    int rc = 0;

    // 1. init params
    TemplateInitParams init_params = DEFAULT_TEMPLATE_INIT_PARAMS;
    memcpy(&sg_dev_info, args, sizeof(sg_dev_info));
    _setup_connect_init_params(&init_params, (DeviceInfo *)args);
    sg_datatemplate_task_exit = 0;

    while (!sg_datatemplate_task_exit) {
        // 2. construct data template client
        void *client = IOT_Template_Construct(&init_params, NULL);
        if (client != NULL) {
            Log_i("Cloud Device Construct Success");
        } else {
            Log_e("Cloud Device Construct Failed");
            HAL_SleepMs(1000);
            continue;
        }

        // 3. start template yield thread
        if (QCLOUD_RET_SUCCESS != IOT_Template_Start_Yield_Thread(client)) {
            Log_e("start template yield thread fail");
            goto exit;
        }

        void *handle = qcloud_iot_trtc_init(client);
        if (handle != NULL) {
            Log_i("trtc handle initial Success");
        } else {
            Log_e("trtc handle initial Failed");
            goto exit;
        }

        while ((IOT_Template_IsConnected(client) || rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT ||
                rc == QCLOUD_RET_MQTT_RECONNECTED || QCLOUD_RET_SUCCESS == rc) &&
               !sg_datatemplate_task_exit) {
            qcloud_iot_trtc_process(client);
            HAL_SleepMs(1000);
        }
    exit:
        IOT_Template_Stop_Yield_Thread(client);
        qcloud_iot_trtc_destroy(handle);
        rc = IOT_Template_Destroy(client);
    }

    return NULL;
}

int IOT_Trtc_Task_Start(DeviceInfo *device_info)
{
    return pthread_create((pthread_t *)&sg_datatemplate_thread, NULL, _datatemplate_task_entry, device_info);
}

void IOT_Trtc_Task_Stop(void)
{
    sg_datatemplate_task_exit = 1;
    pthread_join(*((pthread_t *)&sg_datatemplate_thread), NULL);
}

int IOT_Trtc_Video_Call(void)
{
    return qcloud_iot_trtc_call(VIDEO_CALL);
}

int IOT_Trtc_Audio_Call(void)
{
    return qcloud_iot_trtc_call(AUDIO_CALL);
}

void IOT_Trtc_HangUp(void)
{
    qcloud_iot_trtc_hang_up();
}