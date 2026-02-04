#pragma once
#include "types.h"


#pragma pack(push, 1)

struct Extent {
    SectorID startSector;
    uint32 count;
};


struct InodeDisk {
    uint16 mode;
    uint16 linkCount;
    inodeID inodeId;
    inodeID parentID;
    uint16 uid;
    uint16 gid;

    uint64 size;
    uint32 sectorCount;

    Time atime;
    Time mtime;
    Time ctime;

    Extent direct[12];
    SectorID indirect;
    SectorID doubleIndirect;
    SectorID tripleIndirect;

    SectorID currentSector;
    uint16 offset;

    uint8 padding[90];
};

#pragma pack(pop)

struct InodeLocation {
    SectorID sectorID;
    uint8 slotIndex;

    InodeLocation(SectorID s = 0, uint8 i = 0)
        : sectorID(s), slotIndex(i) {
    }
};

// File Types
enum class inodeType : uint16 {
    FileMode = 0x8000,
    DireMode = 0x4000
};

// Permission Flags
enum class inodeFlags : uint16 {
    OwnerRead = 0x0100, OwnerWrite = 0x0080, OwnerExec = 0x0040,
    GroupRead = 0x0020, GroupWrite = 0x0010, GroupExec = 0x0008,
    OtherRead = 0x0004, OtherWrite = 0x0002, OtherExec = 0x0001,
    None = 0
};

inline inodeFlags operator|(inodeFlags a, inodeFlags b) {
    return static_cast<inodeFlags>(static_cast<uint16>(a) | static_cast<uint16>(b));
}

inline inodeFlags operator|=(inodeFlags& a, inodeFlags b) {
    a = a | b;
    return a;
}

inline inodeFlags operator&(inodeFlags a, inodeFlags b) {
    return static_cast<inodeFlags>(static_cast<uint16>(a) & static_cast<uint16>(b));
}

inline inodeFlags operator&=(inodeFlags& a, inodeFlags b) {
    a = a & b;
    return a;
}

inline inodeFlags operator^(inodeFlags a, inodeFlags b) {
    return static_cast<inodeFlags>(static_cast<uint16>(a) ^ static_cast<uint16>(b));
}

inline inodeFlags operator^=(inodeFlags& a, inodeFlags b) {
    a = a ^ b;
    return a;
}

inline inodeFlags operator~(inodeFlags a) {
    return static_cast<inodeFlags>(~static_cast<uint16>(a));
}

// Bit Masks 
const uint16 S_IFMT = 0xF000; // Type Mask
const uint16 S_IPERM = 0x01FF; // Permission Mask

//static_assert(sizeof(InodeDisk) == 256, "Error: InodeDisk must be exactly 256 bytes!");
//static_assert(offsetof(InodeDisk, direct) == 52, "Error: Direct extents must start at offset 52!");

#define METADATA_SIZE 256 
#define DIRECT_BLOCK_COUNT 12
#define INDIRECT_BLOCK_COUNT 1
#define DOUBLE_INDIRECT_BLOCK_COUNT 1
#define TRIPLE_INDIRECT_BLOCK_COUNT 1