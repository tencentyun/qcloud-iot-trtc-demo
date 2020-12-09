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

#ifndef QCLOUD_IOT_DEVICE_BIND_H_
#define QCLOUD_IOT_DEVICE_BIND_H_

#include "qcloud_iot_export.h"
#include <stdint.h>

int IOT_Device_Bind_Signature_Generate(uint8_t *ouput, uint32_t len, const char *product_id, const char *device_name,
                                       const char *device_secret);

#endif /* QCLOUD_IOT_DEVICE_BIND_H_ */
