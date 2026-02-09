#include "types.h"
#include "SystemKernel.h"
#include "shell.h"
#include "disk_geometry.h"

#include <iostream>
#include <cstring>

int main() {

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
    sb.mapStart = 0;  // Bitmap starts at Sector 1 (Offset 512 bytes)
    sb.inodeStart = 0;  // Inode Table starts at Sector 2 (Offset 1024 bytes)
    sb.dataStart = 0;  // Data starts at Sector 3 (Offset 1536 bytes)
    sb.version = 1;
    sb.freeSpace = (sb.totalSectors - sb.dataStart) * sb.sectorSize;
    std::cout << "Creating device at: " << PATH << "..." << std::endl;
    
    byte* dev = new byte[8192]; 

    memcpy(dev, &sb, sizeof(sb)); 

    Shell shell(new FileSystem(new BlockDevice(dev))); 
    shell.run(); 

    return 0;
}