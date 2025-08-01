#include "GPIOPinModel.hpp"

static const char* TAG = "GPIOPinModel";

// Define ADC constants if not available
#ifndef ADC_CHANNEL_MAX
#define ADC_CHANNEL_MAX ADC_CHANNEL_9
#endif

#ifndef ADC_UNIT_MAX
#define ADC_UNIT_MAX ADC_UNIT_2
#endif

// Constructor
GPIOPinModel::GPIOPinModel(gpio_num_t pin, GPIOMode mode) 
    : pinNumber(pin), currentMode(mode), initialized(false),
      ledcChannel(LEDC_CHANNEL_0), ledcTimer(LEDC_TIMER_0),
      adcHandle(nullptr), adcCaliHandle(nullptr), 
      adcChannel(ADC_CHANNEL_MAX), adcUnit(ADC_UNIT_MAX) {
}

// Destructor
GPIOPinModel::~GPIOPinModel() {
    if (initialized) {
        cleanupPWM();
        cleanupADC();
    }
}

// Initialize the pin according to its mode
bool GPIOPinModel::initialize() {
    if (initialized) {
        return true;
    }

    switch (currentMode) {
        case GPIOMode::INPUT:
            return initializeInput(false, false);
        
        case GPIOMode::INPUT_PULLUP:
            return initializeInput(true, false);
        
        case GPIOMode::INPUT_PULLDOWN:
            return initializeInput(false, true);
        
        case GPIOMode::OUTPUT:
            return initializeOutput();
        
        case GPIOMode::PWM:
            return initializePWM();
        
        case GPIOMode::ANALOGREAD:
            return initializeAnalogRead();
        
        default:
            return false;
    }
}

// Helper method to initialize input modes
bool GPIOPinModel::initializeInput(bool pullup, bool pulldown) {
    gpio_config_t config = {};
    config.pin_bit_mask = (1ULL << pinNumber);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    config.pull_down_en = pulldown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    esp_err_t result = gpio_config(&config);
    if (result == ESP_OK) {
        initialized = true;
        return true;
    } else {
        return false;
    }
}

// Helper method to initialize output mode
bool GPIOPinModel::initializeOutput() {
    gpio_config_t config = {};
    config.pin_bit_mask = (1ULL << pinNumber);
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    esp_err_t result = gpio_config(&config);
    if (result == ESP_OK) {
        initialized = true;
        return true;
    } else {
        return false;
    }
}

// Helper method to initialize PWM mode
bool GPIOPinModel::initializePWM() {
    // Configure LEDC timer
    ledc_timer_config_t timer_config = {};
    timer_config.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_config.timer_num = ledcTimer;
    timer_config.duty_resolution = LEDC_TIMER_10_BIT; // 0-1023 range
    timer_config.freq_hz = 5000; // 5 kHz frequency
    timer_config.clk_cfg = LEDC_AUTO_CLK;

    esp_err_t result = ledc_timer_config(&timer_config);
    if (result != ESP_OK) {
        return false;
    }

    // Configure LEDC channel
    ledc_channel_config_t channel_config = {};
    channel_config.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_config.channel = ledcChannel;
    channel_config.timer_sel = ledcTimer;
    channel_config.intr_type = LEDC_INTR_DISABLE;
    channel_config.gpio_num = pinNumber;
    channel_config.duty = 0;
    channel_config.hpoint = 0;

    result = ledc_channel_config(&channel_config);
    if (result == ESP_OK) {
        initialized = true;
        return true;
    } else {
        return false;
    }
}

// Helper method to initialize analog read mode
bool GPIOPinModel::initializeAnalogRead() {
    adcChannel = getADCChannel(pinNumber);
    adcUnit = getADCUnit(pinNumber);

    if (adcChannel == ADC_CHANNEL_MAX || adcUnit == ADC_UNIT_MAX) {
        return false;
    }

    // Initialize ADC oneshot
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = adcUnit;
    init_config.ulp_mode = ADC_ULP_MODE_DISABLE;

    esp_err_t result = adc_oneshot_new_unit(&init_config, &adcHandle);
    if (result != ESP_OK) {
        return false;
    }

    // Configure ADC channel
    adc_oneshot_chan_cfg_t config = {};
    config.bitwidth = ADC_BITWIDTH_12;
    config.atten = ADC_ATTEN_DB_11; // For 0-3.3V range

    result = adc_oneshot_config_channel(adcHandle, adcChannel, &config);
    if (result != ESP_OK) {
        adc_oneshot_del_unit(adcHandle);
        adcHandle = nullptr;
        return false;
    }

    // Initialize calibration - try line fitting first, fallback to curve fitting
    bool cali_enable = false;
    
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = adcUnit,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    result = adc_cali_create_scheme_line_fitting(&cali_config, &adcCaliHandle);
    if (result == ESP_OK) {
        cali_enable = true;
    }
#endif

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!cali_enable) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = adcUnit,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_12,
        };
        result = adc_cali_create_scheme_curve_fitting(&cali_config, &adcCaliHandle);
        if (result == ESP_OK) {
            cali_enable = true;
        }
    }
#endif

    if (!cali_enable) {
        adcCaliHandle = nullptr;
    }

    initialized = true;
    return true;
}

// Change configuration safely
bool GPIOPinModel::changeConfig(GPIOMode newMode) {
    if (!initialized) {
        return false;
    }

    if (currentMode == newMode) {
        return true;
    }


    // Cleanup current configuration
    cleanupPWM();
    cleanupADC();

    // Reset GPIO to default state
    gpio_reset_pin(pinNumber);

    // Update mode and reinitialize
    currentMode = newMode;
    initialized = false;
    
    return initialize();
}

// Digital read - checks if in input mode
bool GPIOPinModel::digitalRead() {
    if (!initialized) {
        return false;
    }

    if (currentMode != GPIOMode::INPUT && 
        currentMode != GPIOMode::INPUT_PULLUP && 
        currentMode != GPIOMode::INPUT_PULLDOWN) {
        return false;
    }

    return gpio_get_level(pinNumber) == 1;
}

// Digital write - checks if in output mode
bool GPIOPinModel::digitalWrite(bool value) {
    if (!initialized) {
        return false;
    }

    if (currentMode != GPIOMode::OUTPUT) {
        return false;
    }

    esp_err_t result = gpio_set_level(pinNumber, value ? 1 : 0);
    if (result == ESP_OK) {
        return true;
    } else {
        return false;
    }
}

// Analog read for analog read mode
int GPIOPinModel::analogRead() {
    if (!initialized) {
        return -1;
    }

    if (currentMode != GPIOMode::ANALOGREAD) {
        return -1;
    }

    if (!adcHandle) {
        return -1;
    }

    int raw_value;
    esp_err_t result = adc_oneshot_read(adcHandle, adcChannel, &raw_value);
    
    if (result != ESP_OK) {
        return -1;
    }

    // If calibration is available, convert to voltage (mV)
    if (adcCaliHandle) {
        int voltage;
        result = adc_cali_raw_to_voltage(adcCaliHandle, raw_value, &voltage);
        if (result == ESP_OK) {
            return voltage; // Return voltage in mV
        }
    }

    return raw_value; // Return raw ADC value
}

// PWM write for PWM mode
bool GPIOPinModel::pwmWrite(uint32_t dutyCycle) {
    if (!initialized) {
        return false;
    }

    if (currentMode != GPIOMode::PWM) {
        return false;
    }

    // Clamp duty cycle to valid range (0-1023 for 10-bit resolution)
    if (dutyCycle > 1023) {
        dutyCycle = 1023;
    }

    esp_err_t result = ledc_set_duty(LEDC_LOW_SPEED_MODE, ledcChannel, dutyCycle);
    if (result != ESP_OK) {
        return false;
    }

    result = ledc_update_duty(LEDC_LOW_SPEED_MODE, ledcChannel);
    if (result == ESP_OK) {
        return true;
    } else {
        return false;
    }
}

// Cleanup PWM configuration
void GPIOPinModel::cleanupPWM() {
    if (currentMode == GPIOMode::PWM) {
        ledc_stop(LEDC_LOW_SPEED_MODE, ledcChannel, 0);
    }
}

// Cleanup ADC configuration
void GPIOPinModel::cleanupADC() {
    if (adcCaliHandle) {
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_delete_scheme_line_fitting(adcCaliHandle);
#elif ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_delete_scheme_curve_fitting(adcCaliHandle);
#endif
        adcCaliHandle = nullptr;
    }
    
    if (adcHandle) {
        adc_oneshot_del_unit(adcHandle);
        adcHandle = nullptr;
    }
}

// Get ADC channel for given GPIO pin
adc_channel_t GPIOPinModel::getADCChannel(gpio_num_t pin) {
    switch (pin) {
        case GPIO_NUM_1:  return ADC_CHANNEL_0;
        case GPIO_NUM_2:  return ADC_CHANNEL_1;
        case GPIO_NUM_3:  return ADC_CHANNEL_2;
        case GPIO_NUM_4:  return ADC_CHANNEL_3;
        case GPIO_NUM_5:  return ADC_CHANNEL_4;
        case GPIO_NUM_6:  return ADC_CHANNEL_5;
        case GPIO_NUM_7:  return ADC_CHANNEL_6;
        case GPIO_NUM_8:  return ADC_CHANNEL_7;
        case GPIO_NUM_9:  return ADC_CHANNEL_8;
        case GPIO_NUM_10: return ADC_CHANNEL_9;
        case GPIO_NUM_11: return ADC_CHANNEL_0; // ADC2
        case GPIO_NUM_12: return ADC_CHANNEL_1; // ADC2
        case GPIO_NUM_13: return ADC_CHANNEL_2; // ADC2
        case GPIO_NUM_14: return ADC_CHANNEL_3; // ADC2
        case GPIO_NUM_15: return ADC_CHANNEL_4; // ADC2
        case GPIO_NUM_16: return ADC_CHANNEL_5; // ADC2
        case GPIO_NUM_17: return ADC_CHANNEL_6; // ADC2
        case GPIO_NUM_18: return ADC_CHANNEL_7; // ADC2
        case GPIO_NUM_19: return ADC_CHANNEL_8; // ADC2
        case GPIO_NUM_20: return ADC_CHANNEL_9; // ADC2
        default: return ADC_CHANNEL_MAX;
    }
}

// Get ADC unit for given GPIO pin  
adc_unit_t GPIOPinModel::getADCUnit(gpio_num_t pin) {
    if (pin >= GPIO_NUM_1 && pin <= GPIO_NUM_10) {
        return ADC_UNIT_1;
    } else if (pin >= GPIO_NUM_11 && pin <= GPIO_NUM_20) {
        return ADC_UNIT_2;
    }
    return ADC_UNIT_MAX;
}
