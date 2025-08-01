#ifndef GPIOPINMODEL_HPP
#define GPIOPINMODEL_HPP

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "../Enums/GPIOMode.cpp"

// Define ADC constants if not available
#ifndef ADC_CHANNEL_MAX
#define ADC_CHANNEL_MAX ADC_CHANNEL_9
#endif

#ifndef ADC_UNIT_MAX
#define ADC_UNIT_MAX ADC_UNIT_2
#endif

class GPIOPinModel {
private:
    gpio_num_t pinNumber;
    GPIOMode currentMode;
    bool initialized;
    
    // PWM related members
    ledc_channel_t ledcChannel;
    ledc_timer_t ledcTimer;
    
    // ADC related members
    adc_oneshot_unit_handle_t adcHandle;
    adc_cali_handle_t adcCaliHandle;
    adc_channel_t adcChannel;
    adc_unit_t adcUnit;

    // Helper methods
    bool initializeInput(bool pullup, bool pulldown);
    bool initializeOutput();
    bool initializePWM();
    bool initializeAnalogRead();
    void cleanupPWM();
    void cleanupADC();
    adc_channel_t getADCChannel(gpio_num_t pin);
    adc_unit_t getADCUnit(gpio_num_t pin);

public:
    // Constructor
    GPIOPinModel(gpio_num_t pin, GPIOMode mode);
    
    // Destructor
    ~GPIOPinModel();
    
    // Initialization method (must be called after construction)
    bool initialize();
    
    // Change configuration safely
    bool changeConfig(GPIOMode newMode);
    
    // Digital operations
    bool digitalRead();
    bool digitalWrite(bool value);
    
    // Analog operations
    int analogRead();
    
    // PWM operations
    bool pwmWrite(uint32_t dutyCycle); // 0-1023 range
    
    // Getters
    gpio_num_t getPinNumber() const { return pinNumber; }
    GPIOMode getCurrentMode() const { return currentMode; }
    bool isInitialized() const { return initialized; }
};

#endif // GPIOPINMODEL_HPP
