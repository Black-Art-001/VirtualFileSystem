#pragma once

#include "BufferCache.h"
#include "types.h"


class PointerMapManager
{
private:
	BufferCache& bufferCache;
	uint32 entries_per_sector; 
	SectorID pointer_map_start; 
	SectorID reserved; 
	SectorID first_allocatable_sector;
	size_t space; 
	size_t pointerMapSize; 


	struct mapEntry
	{
		inodeID* value;
		SectorID page_id;
		CachePage* page;
		PointerMapManager* parent; 
		friend struct mapEntry;

		mapEntry(SectorID sector_id , PointerMapManager* parent)
		{
			// caculate : find sector 
			SectorID map_index = sector_id - parent->reserved - 1;
			page_id = (map_index / parent->entries_per_sector) + parent->pointer_map_start; // skipe first page (super block)
			page = parent->bufferCache.GetPage(page_id);
			value =	reinterpret_cast<inodeID*>(
				page->data + (map_index % parent->entries_per_sector) * sizeof(inodeID));
		}
		void update() { page->isDirty = true; }
		~mapEntry()
		{
			parent->bufferCache.unpinPage(page_id);
		}
	}; 

public:
	PointerMapManager(BufferCache& buffer);
	
	inodeID getOwner(const SectorID sector_id);
	void setOwner(const SectorID sector_id, const inodeID owner); 
	bool isFree(SectorID sector_id);
	SectorID alloc(inodeID owner);
	void free(SectorID sector_id); 
	size_t freeSpace();
};

