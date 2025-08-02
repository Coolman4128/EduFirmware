#pragma once

#include <cstdint>
#include "../Models/CommandPacket.hpp"
#include "RegisterService.hpp"
#include "HardwareService.hpp"
#include "LinkerService.hpp"

class CommandService {
private:
    RegisterService* registerService;
    HardwareService* hardwareService;
    LinkerService* linkerService;
    
    // Command processing methods
    CommandPacket processReadRegister(const CommandPacket& command);
    CommandPacket processWriteRegister(const CommandPacket& command);
    CommandPacket processReadHardwareConfig(const CommandPacket& command);
    CommandPacket processConfigureHardware(const CommandPacket& command);
    CommandPacket processLinkHardware(const CommandPacket& command);
    CommandPacket processRemoveLinkHardware(const CommandPacket& command);
    
    // Helper methods
    uint8_t gpioModeToConfigByte(const GPIOMode& mode);
    GPIOMode configByteToGPIOMode(uint8_t configByte);
    bool isValidConfigByte(uint8_t configByte);

public:
    static CommandService* lastInstance;
    
    // Constructor
    CommandService(RegisterService* regService, HardwareService* hwService, LinkerService* linkService);
    
    // Destructor
    ~CommandService();
    
    // Get the last instance created
    static CommandService* getLastInstance();
    
    // Main command processing method
    CommandPacket processCommand(const CommandPacket& command);
    
    // Command constants
    static constexpr uint8_t CMD_READ_REGISTER = 0x01;
    static constexpr uint8_t CMD_WRITE_REGISTER = 0x02;
    static constexpr uint8_t CMD_READ_HARDWARE_CONFIG = 0x03;
    static constexpr uint8_t CMD_CONFIGURE_HARDWARE = 0x04;
    static constexpr uint8_t CMD_LINK_HARDWARE = 0x05;
    static constexpr uint8_t CMD_REMOVE_LINK_HARDWARE = 0x06;
    
    // Config byte constants
    static constexpr uint8_t CONFIG_DIGITAL_INPUT = 0x01;
    static constexpr uint8_t CONFIG_DIGITAL_INPUT_PULLUP = 0x02;
    static constexpr uint8_t CONFIG_DIGITAL_INPUT_PULLDOWN = 0x03;
    static constexpr uint8_t CONFIG_DIGITAL_OUTPUT = 0x04;
    static constexpr uint8_t CONFIG_PWM = 0x05;
    static constexpr uint8_t CONFIG_ANALOG_READ = 0x06;
    
    // Hardware type constants
    static constexpr uint8_t HARDWARE_TYPE_GPIO = 0x01;
    static constexpr uint8_t HARDWARE_TYPE_DAC = 0x02;
    
    // Response constants
    static constexpr uint16_t RESPONSE_SUCCESS = 0xAA;
    static constexpr uint16_t RESPONSE_FAILURE = 0xBB;
};
