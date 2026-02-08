#include "types.h"
#include "SystemKernel.h"
#include "shell.h"
#include "disk_geometry.h"

#include <iostream>
#include <cstring>

int main() {
    SystemKernel kernel;

    // Define the Device Parameters
    const size_t DEVICE_SIZE = 8192; // 8 KB
    const std::string PATH = "D:\\temp\\virtual_disk.bin"; 

    // Instantiate and Initialize the Superblock
    Superblock sb{}; 

    std::memcpy(sb.magic, "VFS-DATA", 8);
    // Size Info
    sb.deviceSize = DEVICE_SIZE;
    sb.sectorSize = SECTOR_SIZE;
    sb.totalSectors = DEVICE_SIZE / SECTOR_SIZE; // 16 sectors total 
    sb.mapStart = 1;  // Bitmap starts at Sector 1 (Offset 512 bytes)
    sb.inodeStart = 2;  // Inode Table starts at Sector 2 (Offset 1024 bytes)
    sb.dataStart = 3;  // Data starts at Sector 3 (Offset 1536 bytes)
    sb.version = 1;
    sb.freeSpace = (sb.totalSectors - sb.dataStart) * sb.sectorSize;
    std::cout << "Creating device at: " << PATH << "..." << std::endl;

    if (SystemKernel::NewDevice(DEVICE_SIZE, sb, PATH)) {
        std::cout << "[SUCCESS] Device created successfully." << std::endl;
        std::cout << "  - Total Size: " << sb.deviceSize << " bytes" << std::endl;
        std::cout << "  - Free Space: " << sb.freeSpace << " bytes" << std::endl;
        std::cout << "  - Magic Header: " << std::string(sb.magic, 8) << std::endl;
    }
    else {
        std::cerr << "[ERROR] Failed to create device. Check if the directory exists." << std::endl;
        return -1; 
    }

    int dv = SystemKernel::MountDevice(PATH); 
    int fs = SystemKernel::openFS(dv);
    Shell shell(SystemKernel::getFS(fs)); 
    shell.run(); 

    return 0;
}