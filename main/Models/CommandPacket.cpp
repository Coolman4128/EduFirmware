#include "CommandPacket.hpp"

CommandPacket::CommandPacket(uint8_t command, uint16_t address, uint16_t data, uint16_t deviceId)
{
    
    Command = command;
    Address = address;
    Data = data;
    DeviceId = deviceId;
    setCrc();
}

void CommandPacket::setCrc()
{
    Crc = calculateCrc(*this);
}

uint8_t CommandPacket::calculateCrc(const CommandPacket& packet)
{
    uint8_t crc = 0;
    uint8_t polynomial = 0x07;

    for (uint8_t i = 0; i < 7; ++i) {
        if (i == 0) {
            crc ^= packet.Command;
        }
        if (i == 1) {
            crc ^= (packet.Address & 0xFF);
        }
        if (i == 2) {
            crc ^= (packet.Address >> 8);
        }
        if (i == 3) {
            crc ^= (packet.Data & 0xFF);
        }
        if (i == 4) {
            crc ^= (packet.Data >> 8);
        }
        if (i == 5) {
            crc ^= (packet.DeviceId & 0xFF);
        }
        if (i == 6) {
            crc ^= (packet.DeviceId >> 8);
        }
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
    }
}
    return crc;
}

