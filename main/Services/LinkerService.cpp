#include "LinkerService.hpp"

// Static member initialization
LinkerService* LinkerService::lastInstance = nullptr;

LinkerService::LinkerService(HardwareService* hwService, RegisterService* regService) 
    : hardwareService(hwService), registerService(regService) {
    lastInstance = this;
}

LinkerService::~LinkerService() {
    clearAllLinks();
    if (lastInstance == this) {
        lastInstance = nullptr;
    }
}

LinkerService* LinkerService::getLastInstance() {
    return lastInstance;
}

bool LinkerService::createLink(uint32_t hardwareId, uint16_t registerAddress, bool isInput) {
    // Validate inputs
    if (!validateHardwareId(hardwareId) || !validateRegisterAddress(registerAddress)) {
        return false;
    }
    
    // Remove existing link if it exists (overwrite behavior)
    if (linkExists(hardwareId)) {
        removeLink(hardwareId);
    }
    
    // Create the link
    hardwareToRegister[hardwareId] = registerAddress;
    registerToHardware[registerAddress].insert(hardwareId);
    hardwareInputTypes[hardwareId] = isInput;
    
    return true;
}

bool LinkerService::removeLink(uint32_t hardwareId) {
    if (!linkExists(hardwareId)) {
        return false;
    }
    
    // Get the register address
    uint16_t registerAddress = hardwareToRegister[hardwareId];
    
    // Remove from both maps
    hardwareToRegister.erase(hardwareId);
    registerToHardware[registerAddress].erase(hardwareId);
    
    // Clean up empty register entries
    if (registerToHardware[registerAddress].empty()) {
        registerToHardware.erase(registerAddress);
    }
    
    // Remove input type tracking
    hardwareInputTypes.erase(hardwareId);
    
    return true;
}

bool LinkerService::linkExists(uint32_t hardwareId) const {
    return hardwareToRegister.find(hardwareId) != hardwareToRegister.end();
}

uint16_t LinkerService::getLinkedRegister(uint32_t hardwareId) const {
    auto it = hardwareToRegister.find(hardwareId);
    if (it != hardwareToRegister.end()) {
        return it->second;
    }
    return 0; // Return 0 if not found (could be a valid register, so check linkExists first)
}

std::vector<uint32_t> LinkerService::getLinkedHardware(uint16_t registerAddress) const {
    std::vector<uint32_t> hardware;
    
    auto it = registerToHardware.find(registerAddress);
    if (it != registerToHardware.end()) {
        hardware.reserve(it->second.size());
        for (uint32_t hwId : it->second) {
            hardware.push_back(hwId);
        }
    }
    
    return hardware;
}

bool LinkerService::isHardwareInput(uint32_t hardwareId) const {
    auto it = hardwareInputTypes.find(hardwareId);
    if (it != hardwareInputTypes.end()) {
        return it->second;
    }
    return false; // Default to output if not found
}

std::vector<HardwareLinkInfo> LinkerService::getAllLinks() const {
    std::vector<HardwareLinkInfo> links;
    links.reserve(hardwareToRegister.size());
    
    for (const auto& pair : hardwareToRegister) {
        HardwareLinkInfo info;
        info.hardwareId = pair.first;
        info.registerAddress = pair.second;
        info.hardwareType = hardwareService->getHardwareType(pair.first);
        info.isInput = isHardwareInput(pair.first);
        links.push_back(info);
    }
    
    return links;
}

size_t LinkerService::getLinkCount() const {
    return hardwareToRegister.size();
}

void LinkerService::processInputHardware() {
    if (!hardwareService || !registerService) {
        return;
    }
    
    for (const auto& pair : hardwareToRegister) {
        uint32_t hardwareId = pair.first;
        uint16_t registerAddress = pair.second;
        
        // Only process input hardware
        if (!isHardwareInput(hardwareId)) {
            continue;
        }
        
        HardwareType type = hardwareService->getHardwareType(hardwareId);
        
        switch (type) {
            case HardwareType::GPIO_PIN: {
                auto gpio = hardwareService->getGPIOPin(hardwareId);
                if (!gpio) continue;
                
                GPIOMode mode = gpio->getCurrentMode();
                
                if (mode == GPIOMode::INPUT || mode == GPIOMode::INPUT_PULLUP || mode == GPIOMode::INPUT_PULLDOWN) {
                    // Digital input
                    bool digitalValue = gpio->digitalRead();
                    registerService->writeRegister(registerAddress, digitalValue ? 1 : 0);
                } else if (mode == GPIOMode::ANALOGREAD) {
                    // Analog input
                    int analogValue = gpio->analogRead();
                    // Clamp to 16-bit range
                    uint16_t regValue = (analogValue < 0) ? 0 : 
                                       (analogValue > 65535) ? 65535 : 
                                       static_cast<uint16_t>(analogValue);
                    registerService->writeRegister(registerAddress, regValue);
                }
                break;
            }
            case HardwareType::DAC:
                // DACs are typically output devices, but could be used for feedback reading
                // Implementation depends on specific DAC capabilities
                break;
        }
    }
}

void LinkerService::processOutputHardware() {
    if (!hardwareService || !registerService) {
        return;
    }
    
    for (const auto& pair : hardwareToRegister) {
        uint32_t hardwareId = pair.first;
        uint16_t registerAddress = pair.second;
        
        // Only process output hardware
        if (isHardwareInput(hardwareId)) {
            continue;
        }
        
        // Read the register value
        uint16_t registerValue = registerService->readRegister(registerAddress);
        
        HardwareType type = hardwareService->getHardwareType(hardwareId);
        
        switch (type) {
            case HardwareType::GPIO_PIN: {
                auto gpio = hardwareService->getGPIOPin(hardwareId);
                if (!gpio) continue;
                
                GPIOMode mode = gpio->getCurrentMode();
                
                if (mode == GPIOMode::OUTPUT) {
                    // Digital output
                    bool digitalValue = (registerValue != 0);
                    gpio->digitalWrite(digitalValue);
                } else if (mode == GPIOMode::PWM) {
                    // PWM output - clamp to 0-1023 range
                    uint32_t pwmValue = (registerValue > 1023) ? 1023 : registerValue;
                    gpio->pwmWrite(pwmValue);
                }
                break;
            }
            case HardwareType::DAC: {
                auto dac = hardwareService->getDAC(hardwareId);
                if (!dac) continue;
                
                // Write register value directly to DAC
                dac->writeRaw(registerValue);
                break;
            }
        }
    }
}

void LinkerService::processAllHardware() {
    processInputHardware();
    processOutputHardware();
}

void LinkerService::clearAllLinks() {
    hardwareToRegister.clear();
    registerToHardware.clear();
    hardwareInputTypes.clear();
}

std::vector<uint16_t> LinkerService::getLinkedRegisters() const {
    std::vector<uint16_t> registers;
    registers.reserve(registerToHardware.size());
    
    for (const auto& pair : registerToHardware) {
        registers.push_back(pair.first);
    }
    
    return registers;
}

bool LinkerService::validateHardwareId(uint32_t hardwareId) const {
    return hardwareService && hardwareService->hardwareExists(hardwareId);
}

bool LinkerService::validateRegisterAddress(uint16_t registerAddress) const {
    return registerService && registerAddress < RegisterService::getRegisterCount();
}
