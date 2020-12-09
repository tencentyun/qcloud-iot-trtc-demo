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

#ifndef TRTC_DATATEMPLATE_H_
#define TRTC_DATATEMPLATE_H_

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

int IOT_Trtc_Task_Start(DeviceInfo *device_info);

void IOT_Trtc_Task_Stop(void);

int IOT_Trtc_Video_Call(void);

int IOT_Trtc_Audio_Call(void);

void IOT_Trtc_HangUp(void);

#endif /* TENCENT_SDK_WARPER_H_ */
