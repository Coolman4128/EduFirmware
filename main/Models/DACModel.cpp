#include "DACModel.hpp"

// Constructor for external I2C DAC
DACModel::DACModel(gpio_num_t sda, gpio_num_t scl, uint8_t address, 
                   i2c_port_t port, float maxVolt)
    : i2cPort(port), sdaPin(sda), sclPin(scl), 
      i2cAddress(address), i2cInitialized(false),
      maxRawValue(4095), maxVoltage(maxVolt), minVoltage(0.0f) {
}

// Destructor
DACModel::~DACModel() {
    cleanupI2C();
}

// Initialize the DAC according to its configuration
bool DACModel::initialize() {
    if (sdaPin != GPIO_NUM_NC && sclPin != GPIO_NUM_NC) {
        // Initialize I2C DAC
        return initializeI2C();
    }
    
    return false;
}

// Initialize I2C for external DAC
bool DACModel::initializeI2C() {
    if (i2cInitialized) {
        return true;
    }
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sdaPin,
        .scl_io_num = sclPin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 100000 // 100kHz
        }
    };
    
    esp_err_t ret = i2c_param_config(i2cPort, &conf);
    if (ret != ESP_OK) {
        return false;
    }
    
    ret = i2c_driver_install(i2cPort, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        return false;
    }
    
    i2cInitialized = true;
    return true;
}

// Cleanup I2C
void DACModel::cleanupI2C() {
    if (i2cInitialized) {
        i2c_driver_delete(i2cPort);
        i2cInitialized = false;
    }
}

// Write voltage to DAC
bool DACModel::writeVoltage(float voltage) {
    if (voltage < minVoltage || voltage > maxVoltage) {
        return false;
    }
    
    uint16_t rawValue = voltageToRaw(voltage);
    return writeRaw(rawValue);
}

// Write raw value to DAC
bool DACModel::writeRaw(uint16_t rawValue) {
    if (rawValue > maxRawValue) {
        return false;
    }
    
    if (i2cInitialized) {
        // Write to external I2C DAC
        esp_err_t ret = writeToI2CDAC(rawValue);
        if (ret != ESP_OK) {
            return false;
        }
        return true;
    }
    
    return false;
}

// Write to I2C DAC (assuming MCP4725 or similar protocol)
esp_err_t DACModel::writeToI2CDAC(uint16_t rawValue) {
    if (!i2cInitialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // MCP4725 protocol: write 12-bit value in two bytes
    uint8_t data[2];
    data[0] = (rawValue >> 8) & 0x0F;  // Upper 4 bits
    data[1] = rawValue & 0xFF;         // Lower 8 bits
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2cAddress << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data[0], true);
    i2c_master_write_byte(cmd, data[1], true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

// Convert voltage to raw value
uint16_t DACModel::voltageToRaw(float voltage) const {
    if (voltage < minVoltage) voltage = minVoltage;
    if (voltage > maxVoltage) voltage = maxVoltage;
    
    float ratio = (voltage - minVoltage) / (maxVoltage - minVoltage);
    return (uint16_t)(ratio * maxRawValue);
}

// Convert raw value to voltage
float DACModel::rawToVoltage(uint16_t rawValue) const {
    if (rawValue > maxRawValue) rawValue = maxRawValue;
    
    float ratio = (float)rawValue / maxRawValue;
    return minVoltage + (ratio * (maxVoltage - minVoltage));
}
