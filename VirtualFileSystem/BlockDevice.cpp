#include "BlockDevice.h"
#include <memory>
#include "types.h"

bool BlockDevice::initialize(size_t totalSectors, size_t sectorSize)
{
	device = nullptr; 
	superblock = nullptr; 
	device = new byte[totalSectors * sectorSize ]{};
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
	byte* buffer = new byte[DEFAULT_READ_HEADER_SIZE]; // temporary buffer to avoid alignment issues
	memcpy(buffer, device, 64); // make a copy of superblock to avoid alignment issues
	superblock = reinterpret_cast<Superblock*>(buffer);
	return superblock; 
}

void BlockDevice::free()
{
	if (device)
	{
		delete[]device; 
		device = nullptr; 
	}
	freeSuperblock();
}

void BlockDevice::freeSuperblock()
{
	if (superblock)
	{
		delete[] reinterpret_cast<byte*>(superblock);
		superblock = nullptr;
	}
}

void BlockDevice::saveSuperblock()
{
	memcpy(device, superblock, DEFAULT_READ_HEADER_SIZE);
}

const Superblock *const BlockDevice::getSuperblock() const
{
	return superblock; 
}

Superblock* BlockDevice::getMutableSuperBlock()
{
	return superblock; 
}





