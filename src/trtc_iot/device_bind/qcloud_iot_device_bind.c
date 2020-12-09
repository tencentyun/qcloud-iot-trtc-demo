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

#include "qcloud_iot_device_bind.h"

#include "qrcode_display.h"
#include "utils_sha256.h"
#include "utils_base64.h"

static int utils_hexify(uint8_t *p_str, size_t len, char *u_buf)
{
    char *p = u_buf;
    int   i;

#define CHANGE_TO_HEXCHAR(c) ((c) > 9 ? (c)-10 + 'a' : (c) + '0')

    for (i = 0; i < len; i++) {
        *p       = CHANGE_TO_HEXCHAR(p_str[i] >> 4);
        *(p + 1) = CHANGE_TO_HEXCHAR(p_str[i] & 0xf);
        p += 2;
    }
    *p = 0;
    return 0;
}

int IOT_Device_Bind_Signature_Generate(uint8_t *ouput, uint32_t len, const char *product_id, const char *device_name,
                                       const char *device_secret)
{
    size_t  rlen;
    int8_t  buff[256];
    uint8_t sign[32];
    char    sign_base64[64];
    uint8_t key[32];

    int  random    = rand();
    long timestamp = HAL_Timer_current_sec();

    int info_len = HAL_Snprintf(buff, 256, "%s%s;%d;%ld", product_id, device_name, random, timestamp);
    Log_d("sign_info: %s", buff);
    qcloud_iot_utils_base64decode(key, sizeof(key), &rlen, device_secret, strlen(device_secret));
    utils_hmac_sha256(buff, info_len, sign, key, rlen);
    qcloud_iot_utils_base64encode(sign_base64, sizeof(sign_base64), &rlen, sign, 32);

    // utils_hexify(sign, 32, sign_base64);
    int rc =
        HAL_Snprintf(ouput, len, "%s;%s;%d;%ld;hmacsha256;%s", product_id, device_name, random, timestamp, sign_base64);
    return rc;
}
