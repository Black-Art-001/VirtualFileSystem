#include "BufferCache.h"
#include <algorithm>

BufferCache::BufferCache(BlockDevice* device) :
	device(device)
{
	// constructor implementation
}; 

BufferCache::~BufferCache()
{
	// destructor implementation
}

CachePage* BufferCache::readSector(SectorID sector_id)
{
	auto it = std::find_if(pages.begin(), pages.end(),
		[sector_id](const CachePage& page) { return page.sector_id == sector_id;  });

	if (it != pages.end())
	{
		(*it).pin_count++; // increment pin count
		return &(*it);
	}
	else {
		// sector not found in cache, make base page
		CachePage newPage;
		newPage.sector_id = sector_id;
		newPage.data = new byte[device->getSectorSize()];
		newPage.isDirty = false; 
		newPage.pin_count = 1;

		// load data from block device 
		device->readSector(sector_id, newPage.data);

		// add to pages 
		pages.push_back(newPage);
	}
	
	return &pages.back();
}

void BufferCache::writeSector(SectorID sector_id)
{
	auto it = std::find_if(pages.begin(), pages.end(),
		[sector_id](const CachePage& page) { return page.sector_id == sector_id ;});
	if (it == pages.end())
		return; // not found
	else {
		if ((*it).isDirty) // data has changed ( file handler mark it if it changed 
		{
			device->writeSector(sector_id, (*it).data); // write back to device
			(*it).isDirty = false; // mark as clean 
		}
	}
}

void BufferCache::removePage(SectorID sector_id)
{
	auto it = std::find_if(pages.begin() , pages.end(), 
		[sector_id](const CachePage& page) { return page.sector_id == sector_id });

	if (it == pages.end())
		return; // not found
	else {
		if ((*it).pin_count > 1)
		{ 
			(*it).pin_count--; // decrease pin count number 
		}
		else {
			delete[](*it).data; // free data
			pages.erase(it); // remove from cache
		}
		
	}
}

void BufferCache::SyncAll()
{
	for (auto page : pages)
		if (not page.isDirty)
			if (page.data) 
				device->writeSector(page.sector_id, page.data); 
}

void BufferCache::ClearAll()
{
	for(auto page : pages)
	{
		if (page.data != nullptr)
			delete[]page.data; 
	}
	pages.clear(); // clear all 
}

void BufferCache::flush()
{
	SyncAll(); 
	ClearAll(); 
}


