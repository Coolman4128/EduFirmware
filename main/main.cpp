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
    ESP_LOGI(TAG, "Serial handler task started");
    
    while (1)
    {
        // Listen for command packets with 1 second timeout
        CommandPacket* receivedPacket = serialService->receivePacket(1000);
        
        if (receivedPacket != nullptr)
        {
            ESP_LOGI(TAG, "Received packet - Command: 0x%02X, Address: 0x%04X, Data: 0x%04X, DeviceId: 0x%04X", 
                     receivedPacket->Command, receivedPacket->Address, receivedPacket->Data, receivedPacket->DeviceId);
            
            // Process the command through the command service
            CommandPacket responsePacket = commandService->processCommand(*receivedPacket);
            
            // Send back the response packet
            serialService->sendPacket(&responsePacket);
            
            ESP_LOGI(TAG, "Sent response - Command: 0x%02X, Address: 0x%04X, Data: 0x%04X, DeviceId: 0x%04X", 
                     responsePacket.Command, responsePacket.Address, responsePacket.Data, responsePacket.DeviceId);
            
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
    ESP_LOGI(TAG, "Linker handler task started");
    
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
    ESP_LOGI(TAG, "Starting EDU Firmware");
    
    // Initialize all services in proper order
    ESP_LOGI(TAG, "Initializing services...");
    
    // 1. Initialize Register Service first (no dependencies)
    registerService = new RegisterService();
    ESP_LOGI(TAG, "Register Service initialized");
    
    // 2. Initialize Hardware Service (no dependencies)
    hardwareService = new HardwareService();
    hardwareService->addGPIOPin(0x0001, GPIO_NUM_1, GPIOMode::PWM); // Example GPIO pin
    hardwareService->addGPIOPin(0x0002, GPIO_NUM_2, GPIOMode::ANALOGREAD); // Example GPIO pin
    ESP_LOGI(TAG, "Hardware Service initialized");
    
    // 3. Initialize Linker Service (depends on Hardware and Register services)
    linkerService = new LinkerService(hardwareService, registerService);
    ESP_LOGI(TAG, "Linker Service initialized");
    
    // 4. Initialize Command Service (depends on all other services)
    commandService = new CommandService(registerService, hardwareService, linkerService);
    ESP_LOGI(TAG, "Command Service initialized");
    
    // 5. Initialize Serial Service last
    serialService = new SerialService(115200);
    serialService->Initialize();
    ESP_LOGI(TAG, "Serial Service initialized");
    
    // Verify all services are initialized
    if (!serialService->isInitialized)
    {
        ESP_LOGE(TAG, "Failed to initialize Serial Service");
        return;
    }
    
    ESP_LOGI(TAG, "All services initialized successfully");
    
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
    
    ESP_LOGI(TAG, "Serial handler task created");
    
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
        ESP_LOGE(TAG, "Failed to create linker handler task");
        return;
    }
    
    ESP_LOGI(TAG, "Linker handler task created");
    
    ESP_LOGI(TAG, "EDU Firmware initialization complete");
    ESP_LOGI(TAG, "System ready to receive commands");
    
    // Main task can now idle - the worker tasks will handle everything
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
