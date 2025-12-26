#include "BlockDevice.h"
#include <memory>
#include "types.h"

bool BlockDevice::initialize(size_t totalSectors, size_t sectorSize)
{
	device = nullptr; 

	device = new byte[totalSectors * superblock->sectorSize ]{};

	return device != nullptr; // we can not initialize
}

bool BlockDevice::readSector(SectorID ID, byte* buffer)
{
	memcpy(buffer, device + (ID * superblock->sectorSize), superblock->sectorSize);
	return buffer;
}

bool BlockDevice::writeSector(SectorID ID, const byte* buffer)
{
	memcpy(device + (ID * superblock->sectorSize), buffer, superblock->sectorSize); 
	return true; 
}

bool BlockDevice::readSuperblock()
{
	superblock = reinterpret_cast<Superblock*>(device);
	return superblock; 
}

void BlockDevice::free()
{
	if (device)
	{
		delete[]device; 
		superblock = nullptr; 
		device = nullptr; 
	}
}

uint32 BlockDevice::getSectorSize()
{
	return superblock->sectorSize; 
}






