#include "PointerMapManager.h"
#include "errors.h"
#include <stdexcept>

PointerMapManager::PointerMapManager(BufferCache& buffer) :
	bufferCache(buffer)
{
	pointer_map_start = buffer.info()->mapStart;
	entries_per_sector = buffer.getSectorSize() / sizeof(inodeID);
	double temp = (bufferCache.getTotalSectors() - pointer_map_start) / static_cast<double>(entries_per_sector + 1);
	if (temp - static_cast<int>(temp) < 0.0000001)
		pointerMapSize = temp;
	else
		pointerMapSize = temp + 1; 

	reserved = pointerMapSize + 1; 
	first_allocatable_sector = reserved + 1; // sectors of pointer map + superblock sector   
	space = buffer.getTotalSectors() - reserved;
}

inodeID PointerMapManager::getOwner(SectorID sector_id)
{
	mapEntry entry(sector_id, this); 
	return *entry.value;
}

void PointerMapManager::setOwner(const SectorID sector_id, const inodeID owner)
{
	mapEntry entry(sector_id, this); 
	if (*entry.value == NULL_INODE and owner != NULL_INODE)
	{
		*entry.value = owner; 
		entry.update(); 
		bufferCache.freeSpace()--; // one sector used
		first_allocatable_sector = sector_id;
	}
	else
	{
		if (owner == NULL_INODE)
			bufferCache.freeSpace()++; // one sector freed
		*entry.value = owner; 
		entry.update(); 
	}
}

bool PointerMapManager::isFree(SectorID sector_id)
{

	if (sector_id <= reserved)
		return false;
	else
	{
		auto owner = getOwner(sector_id);
		if (owner == NULL_INODE)
			return true;
		else
			return false;
	}
	// if we reached here , somethings is wrong
	throw std::runtime_error("Failed to check Pointer Map");
}

SectorID PointerMapManager::alloc(inodeID owner)
{
	if (space == bufferCache.freeSpace())
		throw std::runtime_error("there is no free space to allicate"); 
	
	SectorID id = first_allocatable_sector;

	do {
		{
			mapEntry entry(id, this); 
			if (*entry.value == NULL_INODE)
			{
				*entry.value = owner; 
				entry.update(); 
				bufferCache.freeSpace()++; 
				first_allocatable_sector = (id + 1 >= bufferCache.getTotalSectors())
					? reserved + 1 : id + 1;
				return id; 
			}
		}
		id++;
		if (id > bufferCache.getTotalSectors()) // reset
			id = reserved + 1;
	} while (id != first_allocatable_sector); 
	// at first we statrt from one ahead of thefirs_allocatble and after all if we back to it again we move all sectors 
	return NULL_SECTOR; // means we can not allocate !  
}

void PointerMapManager::free(SectorID sector_id)
{
	setOwner(sector_id, NULL_INODE);
}

size_t PointerMapManager::freeSpace()
{
	return bufferCache.freeSpace(); 
}
