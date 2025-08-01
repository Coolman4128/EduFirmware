#ifndef CONNECTIONSERVICE_HPP
#define CONNECTIONSERVICE_HPP

#include "CommandPacket.hpp"

class ConnectionService
{
    public:
        int DeviceId = -1;
        static ConnectionService* getInstance();
        bool isConnected();
        bool attemptConnection(CommandPacket* packet);
        CommandPacket* createHeartbeatPacket();
        ConnectionService();
    private:
        void lookForConnection();
};

#endif 