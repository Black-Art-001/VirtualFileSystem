#pragma once

#include "types.h"

enum class FormatVersion
{
	VFS32,
	FAT,
	NTFS,
	EXT4,
};

#pragma pack(push, 4)
struct Superblock {
    char     magic[8];      // Offset 0: "VFS-DATA"
    uint64 deviceSize;  // offset 8 
    uint32 sectorSize;    // Offset 16
    uint64 totalSectors;  // Offset 20
    uint32 mapStart;      // Offset 28
    uint32 inodeStart;    // Offset 32
    uint32 dataStart;     // Offset 36
    uint16 version;       // Offset 40
    uint64 freeSpace;     // offset 42 
    // end in 50 
    uint8  padding[14];   // Fill to 64 bytes
    
};
#pragma pack(pop)

#define DEFAULT_READ_HEADER_SIZE 64 
#define DEFAULT_SECTOR_SIZE 512


