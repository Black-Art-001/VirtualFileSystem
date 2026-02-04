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
    if (root == NULL_SECTOR) return NULL_SECTOR;
    check_if(level < 1 || level > 3, std::logic_error, "FS Error: Invalid level.");

    uint32 ptrsPerSec = getPtrsPerSector();
    auto page = bc.GetPage(root);
    check_if(page == nullptr, std::runtime_error, "Cache Error: Cannot fetch block.");

    SectorID* ptrs = reinterpret_cast<SectorID*>(page->data);
    SectorID result = NULL_SECTOR;

    if (level == 1) {
        result = (index < ptrsPerSec) ? ptrs[index] : NULL_SECTOR;
    }
    else {
        uint32 childCap = getCapacityAtLevel(level - 1);
        uint32 ptrIndex = index / childCap;
        uint32 remainder = index % childCap;
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
        check_if(root == NULL_SECTOR, std::runtime_error, "Disk full");
        auto page = bc.GetPage(root);
        memset(page->data, 0, bc.getSectorSize());
        page->makeDirty();
        bc.unpinPage(root);
    }

    auto page = bc.GetPage(root);
    SectorID* ptrs = reinterpret_cast<SectorID*>(page->data);

    if (level == 1) {
        ptrs[currentCount] = newSector;
        page->makeDirty();
    }
    else {
        uint32 childCap = getCapacityAtLevel(level - 1);
        uint32 ptrIndex = currentCount / childCap;
        uint32 remainder = currentCount % childCap;
        appendSector(ptrs[ptrIndex], level - 1, remainder, newSector);
        page->makeDirty();
    }
    bc.unpinPage(root);
}

void IndirectBlockManager::freeChain(SectorID root, uint32 level) {
    if (root == NULL_SECTOR) return;

    if (level > 1) {
        auto page = bc.GetPage(root);
        SectorID* ptrs = reinterpret_cast<SectorID*>(page->data);
        uint32 ptrsPerSec = getPtrsPerSector();
        for (uint32 i = 0; i < ptrsPerSec; ++i) {
            if (ptrs[i] != NULL_SECTOR) freeChain(ptrs[i], level - 1);
        }
        bc.unpinPage(root);
    }
    pm.free(root);
}