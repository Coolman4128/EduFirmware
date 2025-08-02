#!/usr/bin/env python3

def calculate_crc(command, address, data, device_id):
    crc = 0
    polynomial = 0x07
    
    # Convert 16-bit values to bytes (little endian)
    bytes_to_process = [
        command,
        address & 0xFF,      # address low byte
        (address >> 8) & 0xFF,  # address high byte
        data & 0xFF,         # data low byte
        (data >> 8) & 0xFF,     # data high byte
        device_id & 0xFF,    # device_id low byte
        (device_id >> 8) & 0xFF # device_id high byte
    ]
    
    print(f"Processing bytes: {[hex(b) for b in bytes_to_process]}")
    
    for i in range(7):
        crc ^= bytes_to_process[i]
        print(f"After XOR with byte {i} (0x{bytes_to_process[i]:02x}): crc = 0x{crc:02x}")
        
        for j in range(8):
            if crc & 0x80:
                crc = (crc << 1) ^ polynomial
            else:
                crc <<= 1
            crc &= 0xFF  # Keep it 8-bit
        print(f"After bit processing: crc = 0x{crc:02x}")
    
    return crc

# Your bytes: 0x05,0x02,0x00,0x01,0x00,0x00,0x00
command = 0x05
address = 0x0002   # bytes 1,2: 0x02,0x00
data = 0x0001      # bytes 3,4: 0x01,0x00  
device_id = 0x0000 # bytes 5,6: 0x00,0x00

print("Input:")
print(f"Command: 0x{command:02x}")
print(f"Address: 0x{address:04x}")
print(f"Data: 0x{data:04x}")
print(f"DeviceId: 0x{device_id:04x}")
print()

crc = calculate_crc(command, address, data, device_id)
print(f"\nFinal CRC: 0x{crc:02x}")
