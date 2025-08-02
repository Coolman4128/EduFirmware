#include "HardwareService.hpp"

// Static member initialization
HardwareService* HardwareService::lastInstance = nullptr;

HardwareService::HardwareService() {
    lastInstance = this;
}

HardwareService::~HardwareService() {
    clearAll();
    if (lastInstance == this) {
        lastInstance = nullptr;
    }
}

HardwareService* HardwareService::getLastInstance() {
    return lastInstance;
}

bool HardwareService::addGPIOPin(uint32_t hardwareId, gpio_num_t pin, GPIOMode mode) {
    // Check if hardware ID already exists
    if (hardwareExists(hardwareId)) {
        // Remove existing hardware first
        removeHardware(hardwareId);
    }
    
    // Create new GPIO pin
    auto gpioPin = std::make_shared<GPIOPinModel>(pin, mode);
    if (!gpioPin->initialize()) {
        return false;
    }
    
    // Store the hardware
    gpioPins[hardwareId] = gpioPin;
    hardwareTypes[hardwareId] = HardwareType::GPIO_PIN;
    
    return true;
}

bool HardwareService::addDAC(uint32_t hardwareId, gpio_num_t sda, gpio_num_t scl, uint8_t address, 
                            i2c_port_t port, float maxVolt) {
    // Check if hardware ID already exists
    if (hardwareExists(hardwareId)) {
        // Remove existing hardware first
        removeHardware(hardwareId);
    }
    
    // Create new DAC
    auto dac = std::make_shared<DACModel>(sda, scl, address, port, maxVolt);
    if (!dac->initialize()) {
        return false;
    }
    
    // Store the hardware
    dacModels[hardwareId] = dac;
    hardwareTypes[hardwareId] = HardwareType::DAC;
    
    return true;
}

bool HardwareService::removeHardware(uint32_t hardwareId) {
    if (!hardwareExists(hardwareId)) {
        return false;
    }
    
    HardwareType type = hardwareTypes[hardwareId];
    
    switch (type) {
        case HardwareType::GPIO_PIN:
            gpioPins.erase(hardwareId);
            break;
        case HardwareType::DAC:
            dacModels.erase(hardwareId);
            break;
    }
    
    hardwareTypes.erase(hardwareId);
    return true;
}

bool HardwareService::hardwareExists(uint32_t hardwareId) const {
    return hardwareTypes.find(hardwareId) != hardwareTypes.end();
}

HardwareType HardwareService::getHardwareType(uint32_t hardwareId) const {
    auto it = hardwareTypes.find(hardwareId);
    if (it != hardwareTypes.end()) {
        return it->second;
    }
    // Return a default value if not found (though this shouldn't happen with proper usage)
    return HardwareType::GPIO_PIN;
}

std::shared_ptr<GPIOPinModel> HardwareService::getGPIOPin(uint32_t hardwareId) {
    auto it = gpioPins.find(hardwareId);
    if (it != gpioPins.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<DACModel> HardwareService::getDAC(uint32_t hardwareId) {
    auto it = dacModels.find(hardwareId);
    if (it != dacModels.end()) {
        return it->second;
    }
    return nullptr;
}

bool HardwareService::readDigitalInput(uint32_t hardwareId, bool& value) {
    if (!hardwareExists(hardwareId) || getHardwareType(hardwareId) != HardwareType::GPIO_PIN) {
        return false;
    }
    
    auto gpio = getGPIOPin(hardwareId);
    if (!gpio) {
        return false;
    }
    
    // Check if the GPIO is configured for input
    GPIOMode mode = gpio->getCurrentMode();
    if (mode != GPIOMode::INPUT && mode != GPIOMode::INPUT_PULLUP && mode != GPIOMode::INPUT_PULLDOWN) {
        return false;
    }
    
    value = gpio->digitalRead();
    return true;
}

bool HardwareService::readAnalogInput(uint32_t hardwareId, int& value) {
    if (!hardwareExists(hardwareId) || getHardwareType(hardwareId) != HardwareType::GPIO_PIN) {
        return false;
    }
    
    auto gpio = getGPIOPin(hardwareId);
    if (!gpio) {
        return false;
    }
    
    // Check if the GPIO is configured for analog read
    if (gpio->getCurrentMode() != GPIOMode::ANALOGREAD) {
        return false;
    }
    
    value = gpio->analogRead();
    return true;
}

bool HardwareService::writeDigitalOutput(uint32_t hardwareId, bool value) {
    if (!hardwareExists(hardwareId) || getHardwareType(hardwareId) != HardwareType::GPIO_PIN) {
        return false;
    }
    
    auto gpio = getGPIOPin(hardwareId);
    if (!gpio) {
        return false;
    }
    
    // Check if the GPIO is configured for output
    if (gpio->getCurrentMode() != GPIOMode::OUTPUT) {
        return false;
    }
    
    return gpio->digitalWrite(value);
}

bool HardwareService::writeAnalogOutput(uint32_t hardwareId, uint16_t value) {
    if (!hardwareExists(hardwareId) || getHardwareType(hardwareId) != HardwareType::DAC) {
        return false;
    }
    
    auto dac = getDAC(hardwareId);
    if (!dac) {
        return false;
    }
    
    return dac->writeRaw(value);
}

bool HardwareService::writePWMOutput(uint32_t hardwareId, uint32_t dutyCycle) {
    if (!hardwareExists(hardwareId) || getHardwareType(hardwareId) != HardwareType::GPIO_PIN) {
        return false;
    }
    
    auto gpio = getGPIOPin(hardwareId);
    if (!gpio) {
        return false;
    }
    
    // Check if the GPIO is configured for PWM
    if (gpio->getCurrentMode() != GPIOMode::PWM) {
        return false;
    }
    
    return gpio->pwmWrite(dutyCycle);
}

size_t HardwareService::getHardwareCount() const {
    return hardwareTypes.size();
}

std::vector<uint32_t> HardwareService::getAllHardwareIds() const {
    std::vector<uint32_t> ids;
    ids.reserve(hardwareTypes.size());
    
    for (const auto& pair : hardwareTypes) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

std::vector<uint32_t> HardwareService::getHardwareIdsByType(HardwareType type) const {
    std::vector<uint32_t> ids;
    
    for (const auto& pair : hardwareTypes) {
        if (pair.second == type) {
            ids.push_back(pair.first);
        }
    }
    
    return ids;
}

void HardwareService::clearAll() {
    gpioPins.clear();
    dacModels.clear();
    hardwareTypes.clear();
}
