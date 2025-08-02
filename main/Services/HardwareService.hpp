#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>
#include "../Models/GPIOPinModel.hpp"
#include "../Models/DACModel.hpp"

// Forward declarations for extensibility
class HardwareBase;

enum class HardwareType {
    GPIO_PIN,
    DAC,
    // Add more hardware types as needed
};

class HardwareService {
private:
    // Storage for different hardware types
    std::unordered_map<uint32_t, std::shared_ptr<GPIOPinModel>> gpioPins;
    std::unordered_map<uint32_t, std::shared_ptr<DACModel>> dacModels;
    
    // Keep track of all hardware IDs and their types for efficient lookup
    std::unordered_map<uint32_t, HardwareType> hardwareTypes;

public:
    static HardwareService* lastInstance;
    
    // Constructor
    HardwareService();
    
    // Destructor
    ~HardwareService();
    
    // Get the last instance created
    static HardwareService* getLastInstance();
    
    // Hardware management methods
    bool addGPIOPin(uint32_t hardwareId, gpio_num_t pin, GPIOMode mode);
    bool addDAC(uint32_t hardwareId, gpio_num_t sda, gpio_num_t scl, uint8_t address = 0x48, 
                i2c_port_t port = I2C_NUM_0, float maxVolt = 3.3f);
    
    bool removeHardware(uint32_t hardwareId);
    bool hardwareExists(uint32_t hardwareId) const;
    HardwareType getHardwareType(uint32_t hardwareId) const;
    
    // Hardware access methods
    std::shared_ptr<GPIOPinModel> getGPIOPin(uint32_t hardwareId);
    std::shared_ptr<DACModel> getDAC(uint32_t hardwareId);
    
    // Hardware operation methods for input devices (read from hardware, write to register)
    bool readDigitalInput(uint32_t hardwareId, bool& value);
    bool readAnalogInput(uint32_t hardwareId, int& value);
    
    // Hardware operation methods for output devices (read from register, write to hardware)
    bool writeDigitalOutput(uint32_t hardwareId, bool value);
    bool writeAnalogOutput(uint32_t hardwareId, uint16_t value);  // For DAC
    bool writePWMOutput(uint32_t hardwareId, uint32_t dutyCycle);  // 0-1023 range
    
    // Utility methods
    size_t getHardwareCount() const;
    std::vector<uint32_t> getAllHardwareIds() const;
    std::vector<uint32_t> getHardwareIdsByType(HardwareType type) const;
    
    // Clear all hardware
    void clearAll();
};
