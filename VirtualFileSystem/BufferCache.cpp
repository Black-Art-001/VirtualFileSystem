#include "BufferCache.h"
#include <stdexcept>

#define	STATIC_CACHE_PAGES // STATIC_CACHE_PAGES for unordered_map reserve
// MAX_PAGE_SIZE for static allocation size exist in types.h

void BufferCache::saveToDevice(CachePage* page)
{
	if (page == nullptr)
	{
		throw std::invalid_argument("CachePage pointer cannot be null");
	}
	else if (page->isDirty == false) {
		return; 
	}
	else if (total_sectors < page->sector_id) {
		throw std::out_of_range("SectorID is out of range of the BlockDevice");
	}
	else {
		device->writeSector(page->sector_id, page->data); 
	}
}

CachePage* BufferCache::loadFromDevice(SectorID sector_id, CachePage* page)
{
	if (page == nullptr)
	{
		throw std::invalid_argument("CachePage pointer cannot be null");
	}
	else if (total_sectors < sector_id) {
		throw std::out_of_range("SectorID is out of range of the BlockDevice");
	}
	else {
		device->readSector(sector_id, page->data); 
		page->sector_id = sector_id; 
		page->isDirty = false; 
		page->pin_count = 1; 
	}

	return page; 
}

void BufferCache::evinctPage(SectorID sector_id)
{
	auto map_it = index.find(sector_id);
	if (map_it == index.end())
		return; // not found 

	auto page_it = map_it->second; // <key , value> 
	delete[] page_it->data;
	page_it->data = nullptr;
	pages.erase(page_it);
	index.erase(map_it);
}


BufferCache::BufferCache(BlockDevice* device) :
	device(device)
{
	if (device == nullptr)
	{
		throw std::invalid_argument("BlockDevice pointer cannot be null");
	}
	if (device->getSuperblock() == nullptr) {
		throw std::runtime_error("BlockDevice is not initialized with a superblock");
	}

#ifdef STATIC_CACHE_PAGES
	index.reserve(MAX_PAGE_SIZE * 1.5); // reserve 1.5times of max pages for better performance 
#endif // STATIC_CACHE_PAGES

	// now we know device is valid
	// get size of cache from superblock
	sector_size = device->getSuperblock()->sectorSize;
	total_sectors = device->getSuperblock()->totalSectors;
	number_of_pages = 0; 
}

void BufferCache::SyncAll()
{
	for (auto& page : pages)
	{
		if (page.isDirty)
		{
			saveToDevice(&page);
			page.isDirty = false; // mark as clean after saving
		}
	}
}

void BufferCache::SyncPage(SectorID sector_id)
{
	auto map_it = index.find(sector_id);
	if (map_it == index.end())
	{
		throw std::runtime_error("CachePage not found in BufferCache");
	}
	else
	{
		auto page_it = map_it->second;
		saveToDevice(&(*page_it));
		page_it->isDirty = false; // mark as clean after saving
	}
}

CachePage* BufferCache::GetPage(SectorID sector_id)
{
	if (sector_id >= total_sectors)
	{
		throw std::out_of_range("SectorID is out of range of the BlockDevice");
	}

	auto map_it = index.find(sector_id);
	if (map_it != index.end()) // we found the page in cache
	{
		auto page_it = map_it->second;
		page_it->pin_count++;
		return &(*page_it);
	}
	else // we didnot find the page in cache
	{
		if (pages.size() >= number_of_pages) // cache is full
		{
			Cleanup();
			if (pages.size() >= number_of_pages) // still full after cleanup
				throw std::runtime_error("BufferCache is full, cannot load new page");
		}
		number_of_pages++;
		pages.emplace_back(sector_id, new byte[sector_size], false, 0);
		index[sector_id] = pages.end();
		return loadFromDevice(sector_id, &(*pages.end()));
	}
	// if we reach here, something went wrong
	throw std::runtime_error("Failed to get or load CachePage");
	return nullptr;
}

void BufferCache::Cleanup()
{
	for (auto it = pages.end(); it != pages.begin(); --it)
	{
		if ((*it).pin_count == 0) // no one is using this page 
		{
			evinctPage((*it).sector_id);
		}
	}
}

void BufferCache::unpinPage(SectorID sector_id)
{
	auto map_it = index.find(sector_id);
	if (map_it == index.end())
	{
		throw std::runtime_error("CachePage not found in BufferCache");
	}
	else
	{
		auto page_it = map_it->second;
		if (page_it->pin_count > 0)
			page_it->pin_count--;
	}
}

void BufferCache::Flush()
{
	SyncAll();
	Cleanup();

}


