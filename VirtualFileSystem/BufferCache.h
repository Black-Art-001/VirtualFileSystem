#pragma once
#include <list>
#include <unordered_map>

#include "BlockDevice.h"
#include "types.h"
#include "rawBit.h"

#define ORDER_MAX_SIZE_CLEANUP -1

#define HASHTABLE_MEM_INCREASE_FACTOR 1.5
#define AUTOCLEAN_SIZE ORDER_MAX_SIZE_CLEANUP
#define AUTOCLEAN_LEN 5
#define FORCEFREE_TRIES true 
class BufferCache; 

class CachePage
{
public:
	CachePage(SectorID id, byte* new_data, bool is_dirty, size_t pin) :
		sector_id(id), data(new_data), isDirty(is_dirty), pin_count(pin) {};
	
	SectorID sector_id; 
	byte* data; 
	bool isDirty;

private:
	size_t pin_count;
	friend class BufferCache;
};

class BufferCache
{
private : 
	BlockDevice* device;
	size_t num_pages;
	std::list<CachePage> pages;
	// we store iterators in the map to make better interpretation with STL containers
	std::unordered_map<SectorID, std::list<CachePage>::iterator> index; // cpIndex
	// cached data 
	int sector_size;
	int total_sectors;

	void saveToDevice(CachePage* page);
	CachePage* loadFromDevice(SectorID sector_id, CachePage* page);
	void evinctPage(SectorID sector_id);

public: 
	BufferCache(BlockDevice* device); // intialize with device
	~BufferCache();

	void SyncAll();
	// target : write
	// write back a page to the device
	void SyncPage(SectorID sector_id);
	// target : read
	// get a page from the cache, load from device if not found
	CachePage* GetPage(SectorID sector_id);
	// target : cleanup
	// free unpinned pages from cache
	// do not save dirty pages
	void Cleanup(int64 size , uint32 num_pages_for_cleanup = -1);
	void unpinPage(SectorID sector_id);
	void Flush();
	void ForceFree();

};

