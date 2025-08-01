#include "RegisterService.hpp"

// Initialize static member
RegisterService* RegisterService::lastInstance = nullptr;

RegisterService::RegisterService() {
    // Initialize all registers to 0
    for (int i = 0; i < 100; i++) {
        registers[i] = 0;
    }
    
    // Update the last instance pointer
    lastInstance = this;
}

RegisterService::~RegisterService() {
    // If this was the last instance, clear the pointer
    if (lastInstance == this) {
        lastInstance = nullptr;
    }
}

RegisterService* RegisterService::getLastInstance() {
    return lastInstance;
}

uint16_t RegisterService::readRegister(uint16_t address) {
    // Check if address is within bounds
    if (address >= 100) {
        return 0; // Return 0 for invalid addresses
    }
    if (address < 0){
        return 0; // Return 0 for negative addresses
    }
    
    return registers[address];
}

bool RegisterService::writeRegister(uint16_t address, uint16_t data) {
    // Check if address is within bounds
    if (address >= 100) {
        return false; // Return false for invalid addresses
    }
    
    registers[address] = data;
    return true; // Return true for successful write
}
