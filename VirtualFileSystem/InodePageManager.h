#pragma once
#include "types.h"
#include "BufferCache.h"
#include "PointerMapManager.h"
#include "InodeDefines.h"

class InodePageManager {
private:
    BufferCache& bc;
    PointerMapManager& pm;

    SectorID startPage;
    uint32 pageCount;

    // Calculated dynamically based on Sector Size
    uint32 inodesPerPage;
    uint32 sectorsPerInodePage;
    uint32 inodesPerSector;

    int findFirstZeroBit(const byte* bitmap, uint32 sizeInBytes);

public:
    InodePageManager(BufferCache& bufferCache, PointerMapManager& pMap, SectorID start);

    SectorID createNewInodePage();
    InodID allocInode();
    InodeLocation getInodeLocation(InodID id);

    uint32 getPageCount() const { return pageCount; }
    uint32 getInodesPerPage() const { return inodesPerPage; }
};