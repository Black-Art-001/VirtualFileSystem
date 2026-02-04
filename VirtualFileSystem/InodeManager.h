#pragma once
#include "types.h"
#include "InodePageManager.h"
#include "IndirectBlockManager.h"

class InodeManager {
public:
    InodeManager(inodeID _id , FileSystem * fs); // getting inodeMan obj from existing inode on disk
    InodeManager(FileSystem* fs); // allocating new inode on disk and get it's inodeMan obj

    InodeDisk* getMutableMetadata() { return metaData; }
    const InodeDisk* getMetadata() const { return metaData; }

    SectorID getSector(uint32 logicalIndex);
    void     appendSector(SectorID newSector);
    void     removeSector(SectorID targetSector);

    Time getMtime() const { return metaData->mtime; }
    Time getAtime() const { return metaData->atime; }
    Time getCtime() const { return metaData->ctime; }
    uint64 getSize() const { return (static_cast<uint64>(metaData->sectorCount)) * (cache.getSectorSize()) + metaData->offset; }
    inodeID getInodeId() const { return metaData->inodeId; }
    uint32 totalSector() const { return metaData->sectorCount; }
    inodeType getType() const;
    inodeFlags getPermison() const;
    uint16 getLinkCount() const { return metaData->linkCount; }
    uint16 getOffset() const { return metaData->offset; }
    inodeID getParentInode() const;

    void updateMtime();
    void updateAtime();
    void unlink();
    void link();
    void setOffset(uint16 offset) { metaData->offset = offset; }
    void setType(inodeType type);
    void setPermison(inodeFlags type);
    void setParentInode(inodeID pInode);

    void syncMetaData();
    void clear(); // Set mata data to zero and free all indirects sectorPointer 
    ~InodeManager();
private:
    InodePageManager& pageManager;
    IndirectBlockManager& ibm;
    PointerMapManager& pm;
    BufferCache& cache;
    inodeID id;
    InodeDisk* metaData;
    InodeLocation location;

    uint32 getPtrsPerSector() const;
    
};