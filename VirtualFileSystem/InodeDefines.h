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
    InodID inodeID;
    InodID parentID;
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
    uint16 Offset;

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


struct StatStruct {
    InodID inodeID;
    uint64 fileSize;
    uint16 mode;
    Time creationTime;
    Time modifiedTime;
    Time accessedTime;
    uint32 sectorCount;
};

//static_assert(sizeof(InodeDisk) == 256, "Error: InodeDisk must be exactly 256 bytes!");
//static_assert(offsetof(InodeDisk, direct) == 52, "Error: Direct extents must start at offset 52!");

#define METADATA_SIZE 256 
#define DIRECT_BLOCK_COUNT 12
#define INDIRECT_BLOCK_COUNT 1
#define DOUBLE_INDIRECT_BLOCK_COUNT 1
#define TRIPLE_INDIRECT_BLOCK_COUNT 1