#pragma once

#include "Types.h"
#include "InodePageManager.h"
#include "IndirectBlockManager.h"
#include "BufferCache.h"
#include "PointerMapManager.h"

class FileSystem;

class InodeManager {
public:
    InodeManager(FileSystem& fs, IndirectBlockManager& indirectMgr, inodeID _id);

    InodeManager(FileSystem& fs, IndirectBlockManager& indirectMgr, inodeType type);

    ~InodeManager();

    // --- Status & Lifecycle ---
    void checkValidity() const;
    void syncMetaData();
    void clear();
    void unlink();

    // --- Mode & Permissions ---
    inodeType getType() const;
    inodeFlags getPermission() const;
    void setPermission(inodeFlags flags);
    bool hasPermission(inodeFlags flag) const;
    void addPermission(inodeFlags flag);
    void removePermission(inodeFlags flag);

    // --- Sector Management ---
    SectorID getSector(uint32 logicalIndex);
    void appendSector(SectorID newSector);
    void updateSize();

    // --- Getters ---
    uint64  getSize() const;
    uint16  getOffset() const;
    inodeID getInodeId() const;
    inodeID getParentID() const;
    uint16  getLinkCount() const;

    // --- Setters ---
    void setOffset(uint16 offset);
    void setParentID(inodeID pID);
    void updateMtime();
    void updateAtime();

    void syncMetaData();
    void clear(); // Set mata data to zero and free all indirects sectorPointer 
    ~InodeManager();
private:
    InodePageManager& pageManager;
    IndirectBlockManager& ibm;
    BufferCache& cache;
    PointerMapManager& pm;

    inodeID id;
    InodeDisk* metaData;
    InodeLocation location;
    bool isValid = true;

    void setType(inodeType type);

    uint32 getPtrsPerSector() const {
        return cache.getSectorSize() / sizeof(SectorID);
    }
};