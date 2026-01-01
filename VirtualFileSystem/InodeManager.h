#pragma once
#include "types.h"
#include "InodePageManager.h"
#include "IndirectBlockManager.h"

class InodeManager {
public:
    InodeManager(InodePageManager& pageMgr, IndirectBlockManager& indirectMgr,
        PointerMapManager& pMap, BufferCache& bc);

    InodID   allocateInode();
    void     readInode(InodID id, InodeDisk& dest);
    void     writeInode(InodID id, const InodeDisk& src);

    SectorID getPhysicalSector(InodID id, uint32 logicalIndex);
    void     appendSectorID(InodID id, SectorID newSector);

    StatStruct getMetaData(InodID id);

private:
    InodePageManager& pageManager;
    IndirectBlockManager& ibm;
    PointerMapManager& pm;
    BufferCache& cache;

    uint32 getPtrsPerSector() const;
};