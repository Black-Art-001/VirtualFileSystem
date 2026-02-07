#include "InodeManager.h"
#include "FileSystem.h"
#include <cstring>
#include <ctime>

InodeManager::InodeManager(FileSystem* fs, inodeID _id)
    : pageManager(fs->getInodePageManager()), ibm(fs->getIndirectBlockManager()),
    cache(fs->getBufferCache()), pm(fs->getPointerMapManager()), id(_id) {

    metaData = new InodeDisk();
    location = pageManager->getInodeLocation(id);
    CachePage* cp = cache->GetPage(location.sectorID);
    memcpy(metaData, cp->data + (location.slotIndex * INODE_SIZE), INODE_SIZE);
    cache->unpinPage(location.sectorID);
	updateAtime();
}

InodeManager::InodeManager(FileSystem* fs, inodeType type)
    : pageManager(fs->getInodePageManager()), ibm(fs->getIndirectBlockManager()),
    cache(fs->getBufferCache()), pm(fs->getPointerMapManager()) {

    id = pageManager->allocInode();
    metaData = new InodeDisk();
    location = pageManager->getInodeLocation(id);
    CachePage* cp = cache->GetPage(location.sectorID);
    memcpy(metaData, cp->data + (location.slotIndex * INODE_SIZE), INODE_SIZE);
    cache->unpinPage(location.sectorID);
    setType(type);
	setPermission(inodeFlags::OwnerRead | inodeFlags::OwnerWrite | inodeFlags::GroupRead | inodeFlags::OtherRead);
    metaData->linkCount = 1;
    metaData->ctime = metaData->mtime = metaData->atime = static_cast<uint64>(std::time(nullptr));
}

InodeManager::~InodeManager() {
    updateAtime();
    syncMetaData();
    delete metaData;
}

void InodeManager::checkValidity() const {
    if (!isValid) throw std::runtime_error("FS_ERROR: Inode object is invalid or unlinked.");
}

// --- Mode Logic (Type & Permissions) ---

inodeType InodeManager::getType() const {
    checkValidity();
    return static_cast<inodeType>(metaData->mode & IM_IFMT);
}

inodeFlags InodeManager::getPermission() const {
    checkValidity();
    return static_cast<inodeFlags>(metaData->mode & IM_IPERM);
}

void InodeManager::setType(inodeType type) {
    checkValidity();
    metaData->mode = (metaData->mode & ~IM_IFMT) | static_cast<uint16>(type) & IM_IFMT;
}

// -- Permission Operators --

void InodeManager::setPermission(inodeFlags flags) {
    checkValidity();
    metaData->mode = (metaData->mode & ~IM_IPERM) | (static_cast<uint16>(flags) & IM_IPERM);
    updateMtime();
}

bool InodeManager::hasPermission(inodeFlags flag) const {
    checkValidity();
    return (static_cast<inodeFlags>(metaData->mode & IM_IPERM) & flag) == flag;
}

void InodeManager::addPermission(inodeFlags flag) {
    checkValidity();
    metaData->mode |= (static_cast<uint16>(flag) & IM_IPERM);
    updateMtime();
}

void InodeManager::removePermission(inodeFlags flag) {
    checkValidity();
    uint16 mask = ~(static_cast<uint16>(flag) & IM_IPERM);
    metaData->mode &= mask;
    updateMtime();
}

// --- Sector Management ---

SectorID InodeManager::getSector(uint32 logicalIndex) {
    checkValidity();
    uint32 offset = 0;

    // 1. Direct Extents (12 entries)
    for (int i = 0; i < 12; i++) {
        if (logicalIndex < offset + metaData->direct[i].count) {
            return metaData->direct[i].startSector + (logicalIndex - offset);
        }
        offset += metaData->direct[i].count;
    }

    uint32 ptrs = getPtrsPerSector();
    uint32 idx = logicalIndex - offset;

    // 2. Single Indirect
    if (idx < ptrs) return ibm->getPhysicalSector(metaData->indirect, 1, idx);
    idx -= ptrs;

    // 3. Double Indirect
    uint32 dblCap = ptrs * ptrs;
    if (idx < dblCap) return ibm->getPhysicalSector(metaData->doubleIndirect, 2, idx);
    idx -= dblCap;

    // 4. Triple Indirect
    return ibm->getPhysicalSector(metaData->tripleIndirect, 3, idx);
}

void InodeManager::appendSector(SectorID newSector) {
    checkValidity();
    if (newSector == NULL_SECTOR) return;

    uint32 directTotal = 0;
    int lastIdx = -1;
    for (int i = 0; i < 12; i++) {
        if (metaData->direct[i].count > 0) lastIdx = i;
        directTotal += metaData->direct[i].count;
    }

    // Try to merge with last direct extent
    if (lastIdx != -1 && newSector == metaData->direct[lastIdx].startSector + metaData->direct[lastIdx].count) {
        metaData->direct[lastIdx].count++;
    }
    // Fill next direct extent
    else if (lastIdx < 11) {
        metaData->direct[lastIdx + 1].startSector = newSector;
        metaData->direct[lastIdx + 1].count = 1;
    }
    // Go to indirect blocks
    else {
        uint32 ptrs = getPtrsPerSector();
        uint32 idx = metaData->sectorCount - directTotal;

        if (idx < ptrs) {
            ibm->appendSector(metaData->indirect, 1, idx, newSector);
        }
        else if (idx < (ptrs + ptrs * ptrs)) {
            ibm->appendSector(metaData->doubleIndirect, 2, idx - ptrs, newSector);
        }
        else {
            ibm->appendSector(metaData->tripleIndirect, 3, idx - (ptrs + ptrs * ptrs), newSector);
        }
    }

    metaData->sectorCount++;
    updateMtime();
}

// --- Lifecycle & Sync ---

void InodeManager::updateSize()
{
    checkValidity();
	metaData->size = getSize();
}

void InodeManager::clear() {
    checkValidity();
    // Free all physical data sectors assigned to this inode
    for (uint32 i = 0; i < metaData->sectorCount; i++) {
        SectorID s = getSector(i);
        if (s != NULL_SECTOR) pm->free(s);
    }

    // Free recursive indirect block structures
    ibm->freeChain(metaData->indirect, 1);
    ibm->freeChain(metaData->doubleIndirect, 2);
    ibm->freeChain(metaData->tripleIndirect, 3);

    // Reset metadata fields
    memset(metaData->direct, 0, sizeof(metaData->direct));
    metaData->indirect = metaData->doubleIndirect = metaData->tripleIndirect = NULL_SECTOR;
    metaData->sectorCount = 0;
    metaData->size = 0;
    metaData->offset = 0;
    updateMtime();
}

void InodeManager::unlink() {
    checkValidity();
    if (metaData->linkCount > 0) metaData->linkCount--;

    if (metaData->linkCount == 0) {
        clear();
        pageManager->freeInode(id);
        isValid = false; // Mark object as destroyed
    }
    else {
        syncMetaData();
    }
}

void InodeManager::link()
{
    metaData->linkCount++;
    syncMetaData();
}

void InodeManager::syncMetaData() {
    checkValidity();
    updateSize();
    CachePage* cp = cache->GetPage(location.sectorID);
    memcpy(cp->data + (location.slotIndex * INODE_SIZE), metaData, INODE_SIZE);
    cp->makeDirty();
    cache->unpinPage(location.sectorID);
}

// --- Standard Setters ---

void InodeManager::setOffset(uint16 offset) {
    checkValidity();
    check_if(offset >= cache->getSectorSize(), std::out_of_range, "Offset too large.")
    metaData->offset = offset;
}

void InodeManager::setParentID(inodeID pID) {
    checkValidity();
    check_if(pID == this->getInodeId() && pID != 0, std::invalid_argument, "Critical: Self-parenting detected!");
    metaData->parentID = pID;
}

void InodeManager::updateMtime() {
    checkValidity();
    metaData->mtime = static_cast<uint64>(std::time(nullptr));
}

void InodeManager::updateAtime() {
    checkValidity();
    metaData->atime = static_cast<uint64>(std::time(nullptr));
}

// --- Basic Getters ---

uint64  InodeManager::getSize() const {
    checkValidity();
    if (metaData->sectorCount == 0) return 0;
    return (static_cast<uint64>(metaData->sectorCount - 1) * cache->getSectorSize()) + metaData->offset;
}

uint16  InodeManager::getOffset() const { checkValidity(); return metaData->offset; }
uint32  InodeManager::getSectorCount() const { checkValidity(); return metaData->sectorCount; }
inodeID InodeManager::getInodeId() const { return id; }
inodeID InodeManager::getParentID() const { checkValidity(); return metaData->parentID; }
uint16  InodeManager::getLinkCount() const { checkValidity(); return metaData->linkCount; }