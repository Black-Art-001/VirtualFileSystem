#pragma once

enum class FormatVersion
{
	VFS32,
	FAT,
	NTFS,
	EXT4,
};


struct Superblock {
    char     magic[8];      // Offset 0: "VFS-DATA"
    uint32_t sectorSize;    // Offset 8
    uint64_t totalSectors;  // Offset 12
    uint32_t mapStart;      // Offset 20
    uint32_t inodeStart;    // Offset 24
    uint32_t dataStart;     // Offset 28
    uint16_t version;       // Offset 32
    uint8_t  padding[30];   // Fill to 64 bytes
};

#define DEFAULT_READ_HEADER_SIZE 64
#define DEFAULT_SECTOR_SIZE 512


