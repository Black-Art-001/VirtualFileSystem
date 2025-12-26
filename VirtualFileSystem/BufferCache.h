#pragma once
#include <vector>

#include "BlockDevice.h"
#include "types.h"
#include "rawBit.h"

struct CachePage
{
	SectorID sector_id; 
	byte* data; 
	bool isDirty;
	size_t pin_count;
};

class BufferCache
{
private : 
	BlockDevice* device;
	size_t num_pages;
	std::vector<CachePage> pages;

public: 
	BufferCache(BlockDevice* device);
	~BufferCache();
	
	CachePage* readSector(SectorID sector_id);
	void writeSector(SectorID sector_id);

	void removePage(SectorID sector_id);
	
	void SyncAll();

	void ClearAll();

	void flush(); 
};

