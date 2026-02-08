#include "SystemKernel.h"
#include "disk_geometry.h"
#include "memory.h"

int SystemKernel::MountDevice(std::string path)
{
	Superblock base_header{};
	// read base data 
	std::fstream file(path, std::ios::in);
	if (not file)
		return false;

	// get size 
	file.seekg(0, std::ios_base::end);
	size_t max = file.tellg();
	file.seekg(0, std::ios_base::beg); // reset tot beginig 
	// read base data
	{
		// get base data 
		byte* temp = new byte[128];

		file.read(reinterpret_cast<char*>(temp), BASE_HEADER_SIZE);
		memcpy(&base_header, BlockDevice(temp).getSuperblock(), BASE_HEADER_SIZE);

		delete[] temp;
	}


	byte* device = new byte[base_header.deviceSize]; 

	// set to beg
	file.seekg(0, std::ios_base::beg); 

	// write in file 
	file.read(reinterpret_cast<char*>(device), max);

	file.close(); 

	BlockDevice* device = new BlockDevice(device); 
}

int SystemKernel::openFS(BlockDevice* device)
{
	FileSystem* fs = new FileSystem(device);

	int counter = 0;
	while (counter++ < fsys.size())
	{
		if (fsys[counter] == nullptr)
		{
			fsys[counter] = fs;
			return counter;
		}
	}
	fsys.push_back(fs);
	return counter;
}

