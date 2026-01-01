#pragma once
#include "types.h"
#include "errors.h"

class BufferCache;
class PointerMapManager;

class IndirectBlockManager {
public:
    IndirectBlockManager(BufferCache& bufferCache, PointerMapManager& pointerMap);

    SectorID getPhysicalSector(SectorID root, uint32 level, uint32 index);
    void appendSector(SectorID& root, uint32 level, uint32 currentCount, SectorID newSector);

private:
    BufferCache& bc;
    PointerMapManager& pm;

    uint32 getPtrsPerSector() const;
    uint32 getCapacityAtLevel(uint32 level) const;
};