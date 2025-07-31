/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "Services/ConnectionService.hpp"

extern "C" void app_main(void)
{
    printf("Hello world!\n");

    ConnectionService connService;
    int result = connService.publicMethod();
    printf("Result from publicMethod: %d\n", result);

    
}
