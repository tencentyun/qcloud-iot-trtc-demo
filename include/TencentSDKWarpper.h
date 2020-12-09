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

#ifndef TENCENT_SDK_WARPPER_H_
#define TENCENT_SDK_WARPPER_H_

#include <stdint.h>

typedef enum { IOT_TRTC_None = 0, IOT_TRTC_EncryptOnly = 1, IOT_TRTC_DecryptOnly = 2 } IOT_Trtc_EncrytionMode;

typedef struct {
    uint32_t               sdk_app_id;
    char*                  user_id;
    char*                  user_sig;
    char*                  str_room_id;
    IOT_Trtc_EncrytionMode mode;
    void*                  user_data;
    int                    av_flag;  // 0:av;1:a
} IOT_Trtc_Params;

/**
 * @brief init trtc
 */
int qcloud_iot_trtc_wrapper_init(void);

/**
 * @brief start trtc
 */
int qcloud_iot_trtc_wrapper_start(IOT_Trtc_Params params);

/**
 * @brief stop trtc
 */
void qcloud_iot_trtc_wrapper_stop(void);

/**
 * @brief trtc usr state
 */
int qcloud_iot_trtc_wrapper_usr_state(void);

#endif /* TENCENT_SDK_WARPER_H_ */
