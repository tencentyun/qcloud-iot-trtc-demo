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

#include "qrcode_display.h"

#include <stdio.h>
#include <stdlib.h>

#include "qrcodegen.h"

#define MAX_QRCODE_VERSION 10

static const char *lt[] = {
    /* 0 */ "  ",
    /* 1 */ "\u2580 ",
    /* 2 */ " \u2580",
    /* 3 */ "\u2580\u2580",
    /* 4 */ "\u2584 ",
    /* 5 */ "\u2588 ",
    /* 6 */ "\u2584\u2580",
    /* 7 */ "\u2588\u2580",
    /* 8 */ " \u2584",
    /* 9 */ "\u2580\u2584",
    /* 10 */ " \u2588",
    /* 11 */ "\u2580\u2588",
    /* 12 */ "\u2584\u2584",
    /* 13 */ "\u2588\u2584",
    /* 14 */ "\u2584\u2588",
    /* 15 */ "\u2588\u2588",
};

void print_qr_char(unsigned char n)
{
    printf("%s", lt[n]);
}

static void printQr(const uint8_t qrcode[])
{
    int           size   = qrcodegen_getSize(qrcode);
    int           border = 2;
    unsigned char num    = 0;

    for (int y = -border; y < size + border; y += 2) {
        for (int x = -border; x < size + border; x += 2) {
            num = 0;

            if (qrcodegen_getModule(qrcode, x, y)) {
                num |= 1 << 0;
            }

            if ((x < size + border) && qrcodegen_getModule(qrcode, x + 1, y)) {
                num |= 1 << 1;
            }

            if ((y < size + border) && qrcodegen_getModule(qrcode, x, y + 1)) {
                num |= 1 << 2;
            }

            if ((x < size + border) && (y < size + border) && qrcodegen_getModule(qrcode, x + 1, y + 1)) {
                num |= 1 << 3;
            }

            print_qr_char(num);
        }

        printf("\n");
    }

    printf("\n");
}

int qrcode_display(const char *text)
{
    enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;
    uint8_t *          qrcode, *tempBuffer;
    int                err = -1;

    qrcode = calloc(1, qrcodegen_BUFFER_LEN_FOR_VERSION(MAX_QRCODE_VERSION));

    if (!qrcode) {
        return -1;
    }

    tempBuffer = calloc(1, qrcodegen_BUFFER_LEN_FOR_VERSION(MAX_QRCODE_VERSION));

    if (!tempBuffer) {
        free(qrcode);
        return -1;
    }

    // Make and print the QR Code symbol
    bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl, qrcodegen_VERSION_MIN, MAX_QRCODE_VERSION,
                                   qrcodegen_Mask_AUTO, true);

    if (ok) {
        printQr(qrcode);
        err = 0;
    }

    free(qrcode);
    free(tempBuffer);
    return err;
}
