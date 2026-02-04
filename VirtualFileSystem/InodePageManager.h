#pragma once
#include "types.h"
#include "BufferCache.h"
#include "PointerMapManager.h"
#include "InodeDefines.h"

class InodePageManager {
public:
    InodePageManager(BufferCache& bufferCache, PointerMapManager& pMap, SectorID start);

    inodeID allocInode();
    bool freeInode(InodeManager& inode);
    InodeLocation getInodeLocation(inodeID id);

    uint32 getPageCount() const { return pageCount; }
    uint32 getInodesPerPage() const { return inodesPerPage; }

private:
    BufferCache& bc;
    PointerMapManager& pm;

    SectorID startPage;
    SectorID endPage;
    uint32 pageCount;

    // Calculated dynamically based on Sector Size
    uint32 inodesPerPage;
    uint32 sectorsPerInodePage;
    uint32 inodesPerSector;

    int findFirstZeroBit(const byte* bitmap, uint32 sizeInBytes);
    SectorID createNewInodePage();
};