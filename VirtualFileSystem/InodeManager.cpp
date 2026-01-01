#include "InodeManager.h"
#include "BufferCache.h"
#include "errors.h"
#include <cstring>

InodeManager::InodeManager(InodePageManager& pageMgr, IndirectBlockManager& indirectMgr,
    PointerMapManager& pMap, BufferCache& bc)
    : pageManager(pageMgr), ibm(indirectMgr), pm(pMap), cache(bc) {
}

uint32 InodeManager::getPtrsPerSector() const {
    return cache.getSectorSize() / sizeof(SectorID);
}

InodID InodeManager::allocateInode() {
    InodID id = pageManager.allocInode();

    if (id == NULL_INODE) {
        if (pageManager.createNewInodePage() != NULL_SECTOR) {
            id = pageManager.allocInode();
        }
    }

    check_if(id == NULL_INODE, std::runtime_error, "FileSystem Error: Inode table is full.");

    InodeDisk empty;
    memset(&empty, 0, sizeof(InodeDisk));
    empty.inodeID = id;
    empty.ctime = empty.mtime = empty.atime = 123456789;

    writeInode(id, empty);
    return id;
}

void InodeManager::readInode(InodID id, InodeDisk& dest) {
    InodeLocation loc = pageManager.getInodeLocation(id);
    CachePage* cp = cache.GetPage(loc.sectorID);
    check_if(cp == nullptr, std::runtime_error, "Disk Read Error: Cannot get Inode sector.");

    memcpy(&dest, cp->data + (loc.slotIndex * INODE_SIZE), INODE_SIZE);
    cache.unpinPage(loc.sectorID);
}

void InodeManager::writeInode(InodID id, const InodeDisk& src) {
    InodeLocation loc = pageManager.getInodeLocation(id);
    CachePage* cp = cache.GetPage(loc.sectorID);
    check_if(cp == nullptr, std::runtime_error, "Disk Write Error: Cannot get Inode sector.");

    memcpy(cp->data + (loc.slotIndex * INODE_SIZE), &src, INODE_SIZE);
    cp->isDirty = true;
    cache.unpinPage(loc.sectorID);
}

SectorID InodeManager::getPhysicalSector(InodID id, uint32 logicalIndex) {
    InodeDisk inode;
    readInode(id, inode);

    //first check the direct's extent
    uint32 offset = 0;
    for (int i = 0; i < DIRECT_BLOCK_COUNT; i++) {
        if (logicalIndex < offset + inode.direct[i].count) {
            return inode.direct[i].startSector + (logicalIndex - offset);
        }
        offset += inode.direct[i].count;
    }

    // if not finded in directs extent , look at indirect pointers
    uint32 ptrs = getPtrsPerSector();
    uint32 idx = logicalIndex - offset;

    if (idx < ptrs) {
        return ibm.getPhysicalSector(inode.indirect, 1, idx);
    }
    idx -= ptrs;

    uint32 doubleCap = ptrs * ptrs;
    if (idx < doubleCap) {
        return ibm.getPhysicalSector(inode.doubleIndirect, 2, idx);
    }
    idx -= doubleCap;

    uint32 tripleCap = ptrs * ptrs * ptrs;
    check_if(idx >= tripleCap, std::out_of_range, "Logical index exceeds maximum file size.");

    return ibm.getPhysicalSector(inode.tripleIndirect, 3, idx);
}

void InodeManager::appendSectorID(InodID id, SectorID newSector) {
    InodeDisk inode;
    readInode(id, inode);

    // first try to append to direct extent's
    int last = -1;
    for (int i = 0; i < DIRECT_BLOCK_COUNT; i++) {
        if (inode.direct[i].count > 0) last = i;
    }

    if (last != -1 && newSector == inode.direct[last].startSector + inode.direct[last].count) {
        inode.direct[last].count++;
    }
    else if (last < (DIRECT_BLOCK_COUNT - 1)) {
        inode.direct[last + 1].startSector = newSector;
        inode.direct[last + 1].count = 1;
    }
    // if cannot append in directs extent , attempt to append indirect pointers
    else {
        uint32 ptrs = getPtrsPerSector();
        uint32 totalDirect = 0;
        for (int i = 0; i < DIRECT_BLOCK_COUNT; i++) 
            totalDirect += inode.direct[i].count;

        uint32 idx = inode.sectorCount - totalDirect;

        if (idx < ptrs) {
            ibm.appendSector(inode.indirect, 1, idx, newSector);
        }
        else if (idx < (ptrs + ptrs * ptrs)) {
            ibm.appendSector(inode.doubleIndirect, 2, idx - ptrs, newSector);
        }
        else {
            ibm.appendSector(inode.tripleIndirect, 3, idx - (ptrs + ptrs * ptrs), newSector);
        }
    }

    inode.sectorCount++;
    writeInode(id, inode);
}

StatStruct InodeManager::getMetaData(InodID id) {
    InodeDisk inode;
    readInode(id, inode);
    return {
        id,
        inode.size,
        inode.mode,
        inode.ctime,
        inode.mtime,
        inode.atime,
        inode.sectorCount
    };
}