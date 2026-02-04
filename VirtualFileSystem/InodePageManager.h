#pragma once
#include "types.h"
#include "BufferCache.h"
#include "PointerMapManager.h"
#include "InodeDefines.h"

class InodePageManager {
public:
    InodePageManager(BufferCache& bufferCache, PointerMapManager& pMap, SectorID start);

    // Core Inode Lifecycle
    inodeID allocInode();
    bool    freeInode(inodeID id);

    // Address Resolution
    InodeLocation getInodeLocation(inodeID id);

    // Helpers
    uint32 getPageCount() const { return pageCount; }
    uint32 getInodesPerPage() const { return inodesPerPage; }

private:
    BufferCache& bc;
    PointerMapManager& pm;

    SectorID startPage;
    SectorID endPage;
    uint32   pageCount;

    // Dynamic Layout Info
    uint32 inodesPerPage;
    uint32 sectorsPerInodePage;
    uint32 inodesPerSector;

    int      findFirstZeroBit(const byte* bitmap, uint32 sizeInBytes);
    SectorID createNewInodePage();
};