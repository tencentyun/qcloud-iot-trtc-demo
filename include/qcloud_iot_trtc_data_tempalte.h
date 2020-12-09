/*
 * Tencent is pleased to support the open source community by making IoT Hub
 available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file
 except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software
 distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND,
 * either express or implied. See the License for the specific language
 governing permissions and
 * limitations under the License.
 *
 */

#ifndef QCLOUD_IOT_TRTC_DATA_TEMPLATE_H_
#define QCLOUD_IOT_TRTC_DATA_TEMPLATE_H_

typedef enum CallType { VIDEO_CALL = 0, AUDIO_CALL } CallType;

/**
 * @brief init trtc
 */
void *qcloud_iot_trtc_init(void *pTemplate_client);

/**
 * @brief destroy trtc
 */
int qcloud_iot_trtc_destroy(void *handle);

/**
 * @brief  trtc process
 */
void qcloud_iot_trtc_process(void *pTemplate_client);

/**
 * @brief  trtc call
 */
int qcloud_iot_trtc_call(CallType type);

/**
 * @brief  trtc hangup
 */
void qcloud_iot_trtc_hang_up(void);

#endif /* TENCENT_SDK_WARPER_H_ */
