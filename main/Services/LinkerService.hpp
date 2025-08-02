#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include "HardwareService.hpp"
#include "RegisterService.hpp"

struct HardwareLinkInfo {
    uint32_t hardwareId;
    uint16_t registerAddress;
    HardwareType hardwareType;
    bool isInput;  // true for input devices (write to register), false for output devices (read from register)
};

class LinkerService {
private:
    // Map from hardware ID to register address
    std::unordered_map<uint32_t, uint16_t> hardwareToRegister;
    
    // Map from register address to set of hardware IDs
    std::unordered_map<uint16_t, std::unordered_set<uint32_t>> registerToHardware;
    
    // Keep track of input/output type for each hardware
    std::unordered_map<uint32_t, bool> hardwareInputTypes;  // true = input, false = output
    
    // References to other services
    HardwareService* hardwareService;
    RegisterService* registerService;

public:
    static LinkerService* lastInstance;
    
    // Constructor
    LinkerService(HardwareService* hwService, RegisterService* regService);
    
    // Destructor
    ~LinkerService();
    
    // Get the last instance created
    static LinkerService* getLastInstance();
    
    // Link management methods
    bool createLink(uint32_t hardwareId, uint16_t registerAddress, bool isInput);
    bool removeLink(uint32_t hardwareId);
    bool linkExists(uint32_t hardwareId) const;
    
    // Query methods
    uint16_t getLinkedRegister(uint32_t hardwareId) const;
    std::vector<uint32_t> getLinkedHardware(uint16_t registerAddress) const;
    bool isHardwareInput(uint32_t hardwareId) const;
    
    // Link information
    std::vector<HardwareLinkInfo> getAllLinks() const;
    size_t getLinkCount() const;
    
    // Processing methods - these will update registers from input hardware and hardware from output registers
    void processInputHardware();   // Read from input hardware, write to registers
    void processOutputHardware();  // Read from registers, write to output hardware
    void processAllHardware();     // Process both input and output
    
    // Utility methods
    void clearAllLinks();
    std::vector<uint16_t> getLinkedRegisters() const;
    
    // Validation methods
    bool validateHardwareId(uint32_t hardwareId) const;
    bool validateRegisterAddress(uint16_t registerAddress) const;
};
