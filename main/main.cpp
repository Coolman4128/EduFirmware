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
#include "esp_log.h"
#include "Services/SerialService.hpp"
#include "Services/RegisterService.hpp"
#include "Services/HardwareService.hpp"
#include "Services/LinkerService.hpp"
#include "Services/CommandService.hpp"
#include "Models/CommandPacket.hpp"
#include "Models/GPIOPinModel.hpp"

static const char* TAG = "EDUFirmware";

// Global service instances
SerialService* serialService = nullptr;
RegisterService* registerService = nullptr;
HardwareService* hardwareService = nullptr;
LinkerService* linkerService = nullptr;
CommandService* commandService = nullptr;

// Task handles
TaskHandle_t serialTaskHandle = nullptr;
TaskHandle_t linkerTaskHandle = nullptr;

// Task for handling serial communication and packet processing
void serialHandlerTask(void* pvParameters)
{
    
    while (1)
    {
        // Listen for command packets with 1 second timeout
        CommandPacket* receivedPacket = serialService->receivePacket(1000);
        
        if (receivedPacket != nullptr)
        {
            // Process the command through the command service
            CommandPacket responsePacket = commandService->processCommand(*receivedPacket);
            vTaskDelay(pdMS_TO_TICKS(5)); // Allow some time for processing
            // Send back the response packet
            serialService->sendPacket(&responsePacket);
            
            // Clean up the received packet
            delete receivedPacket;
        }
        
        // Small delay to prevent task watchdog issues
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Task for handling linker service - propagating all links in a loop
void linkerHandlerTask(void* pvParameters)
{
    
    while (1)
    {
        // Process all hardware links - this will read from input devices and write to registers,
        // and read from registers and write to output devices
        linkerService->processAllHardware();
        
        // Run at a reasonable frequency - 100Hz (10ms delay)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main(void)
{
    // 1. Initialize Register Service first (no dependencies)
    registerService = new RegisterService();
    
    // 2. Initialize Hardware Service (no dependencies)
    hardwareService = new HardwareService();

    // ========================== HARDWARE CONFIG SET UP ==========================

    hardwareService->addGPIOPin(0x0001, GPIO_NUM_1, GPIOMode::PWM); // Example GPIO pin
    hardwareService->addGPIOPin(0x0002, GPIO_NUM_2, GPIOMode::ANALOGREAD); // Example GPIO pin

    // ========================== HARDWARE CONFIG SET UP END ==========================
    
    // 3. Initialize Linker Service (depends on Hardware and Register services)
    linkerService = new LinkerService(hardwareService, registerService);
    
    // 4. Initialize Command Service (depends on all other services)
    commandService = new CommandService(registerService, hardwareService, linkerService);
    
    // 5. Initialize Serial Service last
    serialService = new SerialService(115200);
    serialService->Initialize();
    
    // Verify all services are initialized
    if (!serialService->isInitialized)
    {
        return;
    }
    
    
    // Create the serial handler task (higher priority for responsive communication)
    xTaskCreate(
        serialHandlerTask,      // Function to implement the task
        "SerialHandler",        // Name of the task
        4096,                   // Stack size in words
        NULL,                   // Task input parameter
        5,                      // Priority of the task (higher than linker task)
        &serialTaskHandle       // Task handle
    );
    
    if (serialTaskHandle == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create serial handler task");
        return;
    }
    
    // Create the linker handler task (lower priority, runs continuously)
    xTaskCreate(
        linkerHandlerTask,      // Function to implement the task
        "LinkerHandler",        // Name of the task
        4096,                   // Stack size in words
        NULL,                   // Task input parameter
        3,                      // Priority of the task (lower than serial task)
        &linkerTaskHandle       // Task handle
    );
    
    if (linkerTaskHandle == nullptr)
    {
        return;
    }
    
    // Main task can now idle - the worker tasks will handle everything
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
