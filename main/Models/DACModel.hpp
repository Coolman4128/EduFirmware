#ifndef DACMODEL_HPP
#define DACMODEL_HPP

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_err.h"

class DACModel {
private:
    // I2C configuration for external DAC
    i2c_port_t i2cPort;
    gpio_num_t sdaPin;
    gpio_num_t sclPin;
    uint8_t i2cAddress;
    bool i2cInitialized;
    
    // DAC specifications
    uint16_t maxRawValue;
    float maxVoltage;
    float minVoltage;
    
    // Helper methods
    bool initializeI2C();
    void cleanupI2C();
    esp_err_t writeToI2CDAC(uint16_t rawValue);
    uint16_t voltageToRaw(float voltage) const;
    float rawToVoltage(uint16_t rawValue) const;

public:
    // Constructor for external I2C DAC
    DACModel(gpio_num_t sda, gpio_num_t scl, uint8_t address = 0x48, 
             i2c_port_t port = I2C_NUM_0, float maxVolt = 3.3f);
    
    // Destructor
    ~DACModel();
    
    // Initialization method (must be called after construction)
    bool initialize();
    
    // Core DAC operations
    bool writeVoltage(float voltage);
    bool writeRaw(uint16_t rawValue);
    
    // Configuration getters
    gpio_num_t getSDAPin() const { return sdaPin; }
    gpio_num_t getSCLPin() const { return sclPin; }
    uint8_t getI2CAddress() const { return i2cAddress; }
    i2c_port_t getI2CPort() const { return i2cPort; }
    
    // Status getters
    bool isInitialized() const { return i2cInitialized; }
    
    // DAC specifications getters
    uint16_t getMaxRawValue() const { return maxRawValue; }
    float getMaxVoltage() const { return maxVoltage; }
    float getMinVoltage() const { return minVoltage; }
    
    // Utility methods
    float getCurrentVoltage(uint16_t rawValue) const { return rawToVoltage(rawValue); }
    uint16_t getCurrentRaw(float voltage) const { return voltageToRaw(voltage); }
};

#endif // DACMODEL_HPP
