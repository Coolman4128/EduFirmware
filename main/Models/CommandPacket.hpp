#ifndef COMMANDPACKET_HPP
#define COMMANDPACKET_HPP

#include <cstdint>

class CommandPacket
{
    public:
        uint8_t Command;
        uint16_t Address;
        uint8_t Crc;
        uint16_t Data;
        uint16_t DeviceId;
        CommandPacket(uint8_t command = 0, uint16_t address = 0, uint16_t data = 0, uint16_t deviceId = 0);
        static uint8_t calculateCrc(const CommandPacket& packet);
    private:
        void setCrc();
    };



#endif

