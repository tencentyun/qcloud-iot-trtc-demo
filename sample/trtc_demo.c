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

#include <stdio.h>

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "qrcode_display.h"
#include "qcloud_iot_device_bind.h"
#include "qcloud_iot_trtc.h"

static DeviceInfo sg_devInfo;
// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------
static int sg_main_exit  = 0;
static int sg_input_flag = 1;

static void _main_exit(int sig)
{
    Log_e("demo exit by signal:%d\n", sig);
    sg_main_exit  = 1;
    sg_input_flag = 0;
}

static void _main_reboot(int sig)
{
    pid_t     pid;
    pthread_t tid;

    pid = getpid();
    tid = pthread_self();
    Log_e("vDumpStack %d  pid %u tid %lu", sig, pid, tid);
    sg_main_exit  = 1;
    sg_input_flag = 0;
    // system("reboot"); // define what you need to reboot
}

static int get_key(void)
{
    int key    = 0;
    int result = 0;
    int i      = 1;

    do {
        key = getchar();
        if ((key <= '9') && (key >= '0')) {
            result += result * i + (key - '0');
            i = 10;
            continue;
        }

        if (i != 10) {
            result = 10;
            break;
        }

        if (key == '\n') {
            break;
        }
    } while (sg_input_flag);

    printf("set value %d\n", result);

    return result;
}

int main(int argc, char **argv)
{
    // catch signal
    signal(SIGTERM, _main_exit);
    signal(SIGKILL, _main_exit);
    signal(SIGHUP, _main_exit);
    signal(SIGQUIT, _main_exit);
    signal(SIGINT, _main_exit);
    signal(SIGSEGV, _main_reboot);
    signal(SIGABRT, _main_reboot);
    signal(SIGFPE, _main_reboot);

    IOT_Log_Set_Level(eLOG_DEBUG);

    int ret;

    // 1. get device info
    ret = HAL_GetDevInfo((void *)&sg_devInfo);
    if (QCLOUD_RET_SUCCESS != ret) {
        return ret;
    }

    // 2. print qrcode & bind device
    uint8_t device_bind_info[256];
    ret = IOT_Device_Bind_Signature_Generate(device_bind_info, sizeof(device_bind_info), sg_devInfo.product_id,
                                             sg_devInfo.device_name, sg_devInfo.device_secret);
    if (ret < 0) {
        return ret;
    }

    Log_d("info:%s", device_bind_info);
    qrcode_display(device_bind_info);

    // 3. start trtc task
    ret = IOT_Trtc_Task_Start(&sg_devInfo);
    if (ret) {
        return ret;
    }

    do {
        printf("0: device active request video call!\n");
        printf("1: device active request audio call!\n");
        printf("2: device active disable call!\n");
        printf("3: exit!\n");
        int32_t key = get_key();
        switch (key) {
            case 0:
                IOT_Trtc_Video_Call();
                break;
            case 1:
                IOT_Trtc_Audio_Call();
                break;
            case 2:
                IOT_Trtc_HangUp();
                break;
            case 3:
                sg_input_flag = 0;
                break;
            default:
                break;
        }
    } while (sg_input_flag);
    // 4. idle loop
    // while (!sg_main_exit) {
    //    sleep(1);
    //}

    IOT_Trtc_Task_Stop();
    return 0;
}
