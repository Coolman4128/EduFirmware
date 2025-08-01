#pragma once

#include <cstdint>

class RegisterService {
private:
    
    uint16_t registers[100];

public:
    static RegisterService* lastInstance;
    // Constructor
    RegisterService();
    
    // Destructor
    ~RegisterService();
    
    // Get the last instance created
    static RegisterService* getLastInstance();
    
    // Read from a register at the given address
    uint16_t readRegister(uint16_t address);
    
    // Write to a register at the given address
    bool writeRegister(uint16_t address, uint16_t data);
    
    // Get the total number of registers
    static constexpr uint8_t getRegisterCount() { return 100; }
};
