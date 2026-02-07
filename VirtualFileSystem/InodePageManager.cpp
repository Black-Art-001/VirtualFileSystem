#include "InodePageManager.h"
#include "errors.h"
#include <cstring>
#include <ctime>

InodePageManager::InodePageManager(BufferCache* bufferCache, PointerMapManager* pMap, SectorID start)
    : bc(bufferCache), pm(pMap), startPage(start), endPage(start), pageCount(0) {

    uint32 secSize = bc->getSectorSize();
    inodesPerSector = secSize / INODE_SIZE;
    uint32 bitmapBytes = secSize - sizeof(SectorID);
    inodesPerPage = bitmapBytes * 8;
    sectorsPerInodePage = (inodesPerPage + inodesPerSector - 1) / inodesPerSector;

    SectorID current = startPage;
    while (current != NULL_SECTOR) {
        pageCount++;
        endPage = current;
        CachePage* cp = bc->GetPage(current);
        current = *(reinterpret_cast<SectorID*>(cp->data));
        bc->unpinPage(cp->sector_id);
    }
}

SectorID InodePageManager::createNewInodePage() {
    uint32 totalNeeded = 1 + sectorsPerInodePage;
    SectorID control = pm->allocContiguous(totalNeeded, PAGE_INODE);
    check_if(control == NULL_SECTOR, std::runtime_error, "Disk full: Inode allocation failed");

    CachePage* cp = bc->GetPage(control);
    memset(cp->data, 0, bc->getSectorSize());
    *(reinterpret_cast<SectorID*>(cp->data)) = NULL_SECTOR;
    cp->makeDirty();
    bc->unpinPage(control);

    if (pageCount == 0) {
        startPage = endPage = control;
    }
    else {
        CachePage* last = bc->GetPage(endPage);
        *(reinterpret_cast<SectorID*>(last->data)) = control;
        last->makeDirty();
        bc->unpinPage(endPage);
        endPage = control;
    }
    pageCount++;
    return control;
}

inodeID InodePageManager::allocInode() {
    SectorID current = startPage;
    uint32 pageIdx = 0;

    while (current != NULL_SECTOR) {
        CachePage* cp = bc->GetPage(current);
        byte* bitmap = cp->data + sizeof(SectorID);
        int localBit = findFirstZeroBit(bitmap, bc->getSectorSize() - sizeof(SectorID));

        if (localBit != -1) {
            bitmap[localBit / 8] |= (1 << (localBit % 8));
            cp->makeDirty();
            bc->unpinPage(current);

            inodeID finalId = (pageIdx * inodesPerPage) + localBit + (NULL_INODE + 1);
            SectorID diskSector = current + 1 + (localBit / inodesPerSector);
            uint32 slot = localBit % inodesPerSector;

            CachePage* inodePage = bc->GetPage(diskSector);
            InodeDisk* meta = reinterpret_cast<InodeDisk*>(inodePage->data + (slot * INODE_SIZE));
            memset(meta, 0, INODE_SIZE);
            meta->inodeId = finalId;

            inodePage->makeDirty();
            bc->unpinPage(diskSector);
            return finalId;
        }
        current = *(reinterpret_cast<SectorID*>(cp->data));
        bc->unpinPage(cp->sector_id);
        pageIdx++;
    }

    current = createNewInodePage();
    return allocInode(); // Retry with new page
}

bool InodePageManager::freeInode(inodeID id) {
    inodeID internalIdx = id - (NULL_INODE + 1);
    uint32 pageIdx = internalIdx / inodesPerPage;
    uint32 localBit = internalIdx % inodesPerPage;

    SectorID current = startPage;
    for (uint32 i = 0; i < pageIdx && current != NULL_SECTOR; i++) {
        CachePage* cp = bc->GetPage(current);
        current = *(reinterpret_cast<SectorID*>(cp->data));
        bc->unpinPage(cp->sector_id);
    }

    if (current == NULL_SECTOR) return false;

    CachePage* cp = bc->GetPage(current);
    byte* bitmap = cp->data + sizeof(SectorID);
    bitmap[localBit / 8] &= ~(1 << (localBit % 8));
    cp->makeDirty();
    bc->unpinPage(current);
    return true;
}

InodeLocation InodePageManager::getInodeLocation(inodeID id) {
    inodeID internalIdx = id - (NULL_INODE + 1);
    uint32 pageIdx = internalIdx / inodesPerPage;
    uint32 localIdx = internalIdx % inodesPerPage;

    SectorID current = startPage;
    for (uint32 i = 0; i < pageIdx; i++) {
        CachePage* cp = bc->GetPage(current);
        current = *(reinterpret_cast<SectorID*>(cp->data));
        bc->unpinPage(cp->sector_id);
        check_if(current == NULL_SECTOR, std::out_of_range, "Invalid InodeID");
    }

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