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
#include "Services/SerialService.hpp"
#include "Models/CommandPacket.hpp"
#include "Models/GPIOPinModel.hpp"

extern "C" void app_main(void)
{
    printf("Hello world!\n");

    // Initialize the Serial Service
    SerialService* serialService = new SerialService();
    serialService->Initialize();

    GPIOPinModel* gpioPin = new GPIOPinModel(GPIO_NUM_1, GPIOMode::ANALOGREAD);
    gpioPin->initialize();
    while(true){
        int analogValue = gpioPin->analogRead();
        CommandPacket* packet = new CommandPacket(0x01, 0x1234, (uint16_t)analogValue, 0x9ABC);
        serialService->sendPacket(packet);
        delete packet; // Clean up the packet after sending
        CommandPacket* read = serialService->receivePacket(20000);
        if (read != nullptr) {
            serialService->sendPacket(read);
            delete read; // Clean up the received packet
        }
    }

    delete gpioPin;
    delete serialService;
}
