#include "InodePageManager.h"
#include "errors.h"
#include <cstring>

InodePageManager::InodePageManager(BufferCache& bufferCache, PointerMapManager& pMap, SectorID start)
    : bc(bufferCache), pm(pMap), startPage(start), pageCount(0) {

    uint32 secSize = bc.getSectorSize();

    // Dynamic Geometry Calculations
    inodesPerSector = secSize / INODE_SIZE;
    uint32 bitmapBytes = secSize - sizeof(SectorID); // Subtract NextPage pointer
    inodesPerPage = bitmapBytes * 8;

    // How many data sectors needed for this bitmap's capacity
    sectorsPerInodePage = inodesPerPage / inodesPerSector;

    // Chain traversal to initialize pageCount
    SectorID current = startPage;
    while (current != NULL_SECTOR) {
        pageCount++;
        CachePage* cp = bc.GetPage(current);
        check_if(cp == nullptr, std::runtime_error, "Failed to load Inode control page");

        current = *(reinterpret_cast<SectorID*>(cp->data));
        bc.unpinPage(cp->sector_id);
    }
}

SectorID InodePageManager::createNewInodePage() {
    SectorID control = pm.alloc(PAGE_INODE);
    check_if(control == NULL_SECTOR, std::runtime_error, "Disk full: Cannot allocate Inode control page");

    // Allocate data sectors based on calculated geometry
    for (uint32 i = 0; i < sectorsPerInodePage; i++) {
        SectorID dataSec = pm.alloc(PAGE_INODE);
        check_if(dataSec == NULL_SECTOR, std::runtime_error, "Disk full: Cannot allocate Inode data sector");
    }

    CachePage* cp = bc.GetPage(control);
    memset(cp->data, 0, bc.getSectorSize()); // Clears nextPage and Bitmap
    cp->isDirty = true;
    bc.unpinPage(control);

    // Linking logic
    if (startPage == NULL_SECTOR) {
        startPage = control;
    }
    else {
        SectorID curr = startPage;
        while (true) {
            CachePage* p = bc.GetPage(curr);
            SectorID* nextPtr = reinterpret_cast<SectorID*>(p->data);
            if (*nextPtr == NULL_SECTOR) {
                *nextPtr = control;
                p->isDirty = true;
                bc.unpinPage(curr);
                break;
            }
            SectorID next = *nextPtr;
            bc.unpinPage(curr);
            curr = next;
        }
    }
    pageCount++;
    return control;
}

InodID InodePageManager::allocInode() {
    SectorID current = startPage;
    uint32 pageIdx = 0;

    while (current != NULL_SECTOR) {
        CachePage* cp = bc.GetPage(current);
        byte* bitmap = cp->data + sizeof(SectorID);
        int localBit = findFirstZeroBit(bitmap, bc.getSectorSize() - sizeof(SectorID));

        if (localBit != -1) {
            bitmap[localBit / 8] |= (1 << (localBit % 8));
            cp->isDirty = true;
            bc.unpinPage(current);
            return (pageIdx * inodesPerPage) + localBit;
        }
        SectorID next = *(reinterpret_cast<SectorID*>(cp->data));
        bc.unpinPage(current);
        current = next;
        pageIdx++;
    }
    return NULL_INODE;
}

InodeLocation InodePageManager::getInodeLocation(InodID id) {
    uint32 pageIdx = id / inodesPerPage;
    uint32 localIdx = id % inodesPerPage;
    SectorID current = startPage;

    uint32 maxPossibleInodes = pageCount * inodesPerPage;
    check_if(id >= maxPossibleInodes, std::out_of_range, "InodeID is out of range");

    for (uint32 i = 0; i < pageIdx; i++) {
        CachePage* cp = bc.GetPage(current);
        check_if(cp == nullptr, std::runtime_error, "Failed to iterate Inode pages");
        current = *(reinterpret_cast<SectorID*>(cp->data));
        bc.unpinPage(cp->sector_id);
    }

    // current is control page, data starts from current + 1
    SectorID diskSector = current + 1 + (localIdx / inodesPerSector);
    return InodeLocation(diskSector, static_cast<uint8>(localIdx % inodesPerSector));
}

int InodePageManager::findFirstZeroBit(const byte* bitmap, uint32 size) {
    for (uint32 i = 0; i < size; i++) {
        if (bitmap[i] != 0xFF) {
            for (int b = 0; b < 8; b++) {
                if (!(bitmap[i] & (1 << b))) return (i * 8) + b;
            }
        }
    }
    return -1;
}