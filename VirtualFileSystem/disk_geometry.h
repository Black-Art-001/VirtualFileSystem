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
    uint32 sectorSize;    // Offset 8
    uint64 totalSectors;  // Offset 12
    uint32 mapStart;      // Offset 20
    uint32 inodeStart;    // Offset 24
    uint32 dataStart;     // Offset 28
    uint16 version;       // Offset 32
    uint64 freeSpace; 
    uint8  padding[22];   // Fill to 64 bytes
};
#pragma pack(pop)

#define DEFAULT_READ_HEADER_SIZE 64 
#define DEFAULT_SECTOR_SIZE 512


