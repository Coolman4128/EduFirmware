#ifndef GPIOMODE_HPP
#define GPIOMODE_HPP

enum class GPIOMode {
    INPUT,
    OUTPUT,
    INPUT_PULLUP,
    INPUT_PULLDOWN,
    PWM,
    ANALOGREAD
};

#endif // GPIOMODE_HPP
