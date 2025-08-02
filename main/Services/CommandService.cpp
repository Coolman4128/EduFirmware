#include "CommandService.hpp"
#include "../Enums/GPIOMode.hpp"

// Static member initialization
CommandService* CommandService::lastInstance = nullptr;

CommandService::CommandService(RegisterService* regService, HardwareService* hwService, LinkerService* linkService)
    : registerService(regService), hardwareService(hwService), linkerService(linkService) {
    lastInstance = this;
}

CommandService::~CommandService() {
    if (lastInstance == this) {
        lastInstance = nullptr;
    }
}

CommandService* CommandService::getLastInstance() {
    return lastInstance;
}

CommandPacket CommandService::processCommand(const CommandPacket& command) {
    switch (command.Command) {
        case CMD_READ_REGISTER:
            return processReadRegister(command);
        case CMD_WRITE_REGISTER:
            return processWriteRegister(command);
        case CMD_READ_HARDWARE_CONFIG:
            return processReadHardwareConfig(command);
        case CMD_CONFIGURE_HARDWARE:
            return processConfigureHardware(command);
        case CMD_LINK_HARDWARE:
            return processLinkHardware(command);
        case CMD_REMOVE_LINK_HARDWARE:
            return processRemoveLinkHardware(command);
        default:
            // Return error response for unknown command
            return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
}

CommandPacket CommandService::processReadRegister(const CommandPacket& command) {
    uint16_t registerValue = registerService->readRegister(command.Address);
    return CommandPacket(command.Command, command.Address, registerValue, command.DeviceId);
}

CommandPacket CommandService::processWriteRegister(const CommandPacket& command) {
    bool success = registerService->writeRegister(command.Address, command.Data);
    if (success) {
        return CommandPacket(command.Command, command.Address, 0, command.DeviceId);
    } else {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
}

CommandPacket CommandService::processReadHardwareConfig(const CommandPacket& command) {
    uint32_t hardwareId = static_cast<uint32_t>(command.Address);
    
    // Special case: Address 0x0000 returns total number of hardware devices
    if (command.Address == 0x0000) {
        uint16_t deviceCount = static_cast<uint16_t>(hardwareService->getHardwareCount());
        return CommandPacket(command.Command, command.Address, deviceCount, command.DeviceId);
    }
    
    // Check if hardware exists
    if (!hardwareService->hardwareExists(hardwareId)) {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
    
    HardwareType hwType = hardwareService->getHardwareType(hardwareId);
    uint8_t hardwareTypeByte;
    uint8_t configByte = 0;
    
    if (hwType == HardwareType::GPIO_PIN) {
        hardwareTypeByte = HARDWARE_TYPE_GPIO;
        auto gpioPin = hardwareService->getGPIOPin(hardwareId);
        if (gpioPin) {
            configByte = gpioModeToConfigByte(gpioPin->getCurrentMode());
        }
    } else if (hwType == HardwareType::DAC) {
        hardwareTypeByte = HARDWARE_TYPE_DAC;
        // DAC doesn't have configuration modes like GPIO
        configByte = 0x00;
    } else {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
    
    // Pack hardware type in upper byte, config in lower byte
    uint16_t responseData = (static_cast<uint16_t>(hardwareTypeByte) << 8) | configByte;
    return CommandPacket(command.Command, command.Address, responseData, command.DeviceId);
}

CommandPacket CommandService::processConfigureHardware(const CommandPacket& command) {
    uint32_t hardwareId = static_cast<uint32_t>(command.Address);
    uint8_t configByte = static_cast<uint8_t>(command.Data & 0xFF);
    
    // Check if hardware exists
    if (!hardwareService->hardwareExists(hardwareId)) {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
    
    HardwareType hwType = hardwareService->getHardwareType(hardwareId);
    
    // DAC hardware cannot be reconfigured
    if (hwType == HardwareType::DAC) {
        return CommandPacket(command.Command, command.Address, RESPONSE_SUCCESS, command.DeviceId);
    }
    
    // Only GPIO hardware can be configured
    if (hwType != HardwareType::GPIO_PIN) {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
    
    // Validate config byte
    if (!isValidConfigByte(configByte)) {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
    
    auto gpioPin = hardwareService->getGPIOPin(hardwareId);
    if (!gpioPin) {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
    
    GPIOMode newMode = configByteToGPIOMode(configByte);
    bool success = gpioPin->changeConfig(newMode);
    
    if (success) {
        return CommandPacket(command.Command, command.Address, RESPONSE_SUCCESS, command.DeviceId);
    } else {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
}

CommandPacket CommandService::processLinkHardware(const CommandPacket& command) {
    uint32_t hardwareId = static_cast<uint32_t>(command.Address);
    uint16_t registerAddress = command.Data;
    
    // Check if hardware exists
    if (!hardwareService->hardwareExists(hardwareId)) {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
    
    // Determine if this is an input or output device based on hardware type and configuration
    HardwareType hwType = hardwareService->getHardwareType(hardwareId);
    bool isInput = false;
    
    if (hwType == HardwareType::GPIO_PIN) {
        auto gpioPin = hardwareService->getGPIOPin(hardwareId);
        if (gpioPin) {
            GPIOMode mode = gpioPin->getCurrentMode();
            // Input modes: INPUT, INPUT_PULLUP, INPUT_PULLDOWN, ANALOGREAD
            isInput = (mode == GPIOMode::INPUT || mode == GPIOMode::INPUT_PULLUP || 
                      mode == GPIOMode::INPUT_PULLDOWN || mode == GPIOMode::ANALOGREAD);
        }
    } else if (hwType == HardwareType::DAC) {
        // DAC is always an output device
        isInput = false;
    }
    
    bool success = linkerService->createLink(hardwareId, registerAddress, isInput);
    
    if (success) {
        return CommandPacket(command.Command, command.Address, RESPONSE_SUCCESS, command.DeviceId);
    } else {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
}

CommandPacket CommandService::processRemoveLinkHardware(const CommandPacket& command) {
    uint32_t hardwareId = static_cast<uint32_t>(command.Address);
    
    // removeLink returns true for both successful removal and no existing link
    bool success = linkerService->removeLink(hardwareId);
    
    if (success) {
        return CommandPacket(command.Command, command.Address, RESPONSE_SUCCESS, command.DeviceId);
    } else {
        return CommandPacket(command.Command, command.Address, RESPONSE_FAILURE, command.DeviceId);
    }
}

uint8_t CommandService::gpioModeToConfigByte(const GPIOMode& mode) {
    switch (mode) {
        case GPIOMode::INPUT:
            return CONFIG_DIGITAL_INPUT;
        case GPIOMode::INPUT_PULLUP:
            return CONFIG_DIGITAL_INPUT_PULLUP;
        case GPIOMode::INPUT_PULLDOWN:
            return CONFIG_DIGITAL_INPUT_PULLDOWN;
        case GPIOMode::OUTPUT:
            return CONFIG_DIGITAL_OUTPUT;
        case GPIOMode::PWM:
            return CONFIG_PWM;
        case GPIOMode::ANALOGREAD:
            return CONFIG_ANALOG_READ;
        default:
            return 0x00; // Unknown/invalid mode
    }
}

GPIOMode CommandService::configByteToGPIOMode(uint8_t configByte) {
    switch (configByte) {
        case CONFIG_DIGITAL_INPUT:
            return GPIOMode::INPUT;
        case CONFIG_DIGITAL_INPUT_PULLUP:
            return GPIOMode::INPUT_PULLUP;
        case CONFIG_DIGITAL_INPUT_PULLDOWN:
            return GPIOMode::INPUT_PULLDOWN;
        case CONFIG_DIGITAL_OUTPUT:
            return GPIOMode::OUTPUT;
        case CONFIG_PWM:
            return GPIOMode::PWM;
        case CONFIG_ANALOG_READ:
            return GPIOMode::ANALOGREAD;
        default:
            return GPIOMode::INPUT; // Default to input if invalid
    }
}

bool CommandService::isValidConfigByte(uint8_t configByte) {
    return (configByte >= CONFIG_DIGITAL_INPUT && configByte <= CONFIG_ANALOG_READ);
}
