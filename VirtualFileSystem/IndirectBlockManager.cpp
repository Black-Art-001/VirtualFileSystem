#include "IndirectBlockManager.h"
#include "BufferCache.h"
#include "PointerMapManager.h"
#include <cstring>

IndirectBlockManager::IndirectBlockManager(BufferCache& bufferCache, PointerMapManager& pointerMap)
    : bc(bufferCache), pm(pointerMap) {
}

uint32 IndirectBlockManager::getPtrsPerSector() const {
    return bc.getSectorSize() / sizeof(SectorID);
}

uint32 IndirectBlockManager::getCapacityAtLevel(uint32 level) const {
    uint32 ptrs = getPtrsPerSector();
    uint32 capacity = 1;
    for (uint32 i = 0; i < level; ++i) capacity *= ptrs;
    return capacity;
}

SectorID IndirectBlockManager::getPhysicalSector(SectorID root, uint32 level, uint32 index) {
    check_if(root == NULL_SECTOR, std::runtime_error, "FS Error: Read from NULL indirect block.");
    check_if(level < 1 || level > 3, std::logic_error, "FS Error: Invalid level.");

    uint32 ptrsPerSec = getPtrsPerSector();
    auto page = bc.GetPage(root);
    check_if(page == nullptr, std::runtime_error, "Cache Error: Cannot fetch indirect block.");

    SectorID* ptrs = reinterpret_cast<SectorID*>(page->data);
    SectorID result = NULL_SECTOR;

    if (level == 1) {
        check_if(index >= ptrsPerSec, std::out_of_range, "FS Error: Index exceeds single capacity.");
        result = ptrs[index];
    }
    else {
        uint32 childCap = getCapacityAtLevel(level - 1);
        uint32 ptrIndex = index / childCap;
        uint32 remainder = index % childCap;

        check_if(ptrIndex >= ptrsPerSec, std::out_of_range, "FS Error: Index exceeds tree depth.");

        SectorID nextBlock = ptrs[ptrIndex];
        bc.unpinPage(root);
        return getPhysicalSector(nextBlock, level - 1, remainder);
    }

    bc.unpinPage(root);
    return result;
}

void IndirectBlockManager::appendSector(SectorID& root, uint32 level, uint32 currentCount, SectorID newSector) {
    uint32 ptrsPerSec = getPtrsPerSector();

    if (root == NULL_SECTOR) {
        root = pm.alloc(SYSTEM);
        check_if(root == NULL_SECTOR, std::runtime_error, "Disk full: Cannot alloc indirect block.");

        auto page = bc.GetPage(root);
        SectorID* buffer = reinterpret_cast<SectorID*>(page->data);
        for (uint32 i = 0; i < ptrsPerSec; ++i) buffer[i] = NULL_SECTOR;
        page->isDirty = true;
        bc.unpinPage(root);
    }

    auto page = bc.GetPage(root);
    SectorID* ptrs = reinterpret_cast<SectorID*>(page->data);

    if (level == 1) {
        check_if(currentCount >= ptrsPerSec, std::out_of_range, "FS Error: Single level overflow.");
        ptrs[currentCount] = newSector;
        page->isDirty = true;
    }
    else {
        uint32 childCap = getCapacityAtLevel(level - 1);
        uint32 ptrIndex = currentCount / childCap;
        uint32 remainder = currentCount % childCap;

        SectorID childID = ptrs[ptrIndex];
        SectorID originalID = childID;

        bc.unpinPage(root);
        appendSector(childID, level - 1, remainder, newSector);

        if (childID != originalID) {
            page = bc.GetPage(root);
            ptrs = reinterpret_cast<SectorID*>(page->data);
            ptrs[ptrIndex] = childID;
            page->isDirty = true;
        }
    }
    bc.unpinPage(root);
}