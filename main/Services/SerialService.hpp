#ifndef SERIALSERVICE_HPP
#define SERIALSERVICE_HPP

#include "CommandPacket.hpp"

class SerialService
{
    public:
        int baudRate = 115200;
        bool isInitialized = false;
        static SerialService* Instance;
        void sendPacket(CommandPacket* packet);
        CommandPacket* receivePacket(int timeout = 1000);
        SerialService(int baudRate = 115200);
        void Initialize();
    private:
        
};


#endif 