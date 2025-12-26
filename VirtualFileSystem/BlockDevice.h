#pragma once
#include "disk_geometry.h" 
#include "rawBit.h"
#include "types.h"

using namespace rawBit; 

class BlockDevice
{
private:
	byte* device;
	size_t totalSectors;
	size_t sectorSize; 
	Superblock * superblock;

public:
	bool initialize(size_t totalSectors, size_t sectorSize);
	// bool open(); 
	bool readSector(SectorID ID , byte* buffer);
	bool writeSector(SectorID ID, const byte* buffer);
	// bool getDeviceInfo();
	bool readSuperblock();
	void free();
	uint32 getSectorSize(); 
};

