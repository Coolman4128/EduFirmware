#include "SerialService.hpp"
#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "esp_mac.h"

// Define the static member
SerialService* SerialService::Instance = nullptr;

SerialService::SerialService(int baudRate) {
    this->baudRate = baudRate;
    this->Instance = this;
}

void SerialService::Initialize() {
    const uart_config_t uart_config = {
        .baud_rate = baudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {}
    };
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    this->isInitialized = true;
}

void SerialService::sendPacket(CommandPacket* packet) {
    if (!isInitialized) {
        return;
    }
    uint8_t data[8];
    CommandPacket cmdPacket = *packet;
    data[0] = cmdPacket.Command;
    data[1] = cmdPacket.Address & 0xFF;
    data[2] = (cmdPacket.Address >> 8) & 0xFF;
    data[3] = cmdPacket.Data & 0xFF;
    data[4] = (cmdPacket.Data >> 8) & 0xFF;
    data[5] = cmdPacket.DeviceId & 0xFF;
    data[6] = (cmdPacket.DeviceId >> 8) & 0xFF;
    data[7] = cmdPacket.Crc;
    uart_write_bytes(UART_NUM_0, (const char*)data, sizeof(data));
}

CommandPacket* SerialService::receivePacket(int timeout) {
    if (!isInitialized) {
        return nullptr;
    }
    uint8_t data[8];
    int len = uart_read_bytes(UART_NUM_0, data, sizeof(data), timeout / portTICK_PERIOD_MS);
    if (len == sizeof(data)) {
        CommandPacket* packet = new CommandPacket();
        packet->Command = data[0];
        packet->Address = (data[1] | (data[2] << 8));
        packet->Data = (data[3] | (data[4] << 8));
        packet->DeviceId = (data[5] | (data[6] << 8));
        packet->Crc = data[7];

        // Validate CRC
        if (packet->Crc != CommandPacket::calculateCrc(*packet)) {
            delete packet;
            return nullptr; // CRC mismatch, discard packet
        }
        return packet;
    }
    return nullptr;
}
