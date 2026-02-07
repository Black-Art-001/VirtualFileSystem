#pragma once

#include "types.h"
#include "FileSystem.h"
#include "BlockDevice.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>

#define BASE_HEADER_SIZE 128

class SystemKernel
{
private : 
	std::vector<BlockDevice*> devices; 
	std::vector<FileSystem*> fsys; 
	std::unordered_map<int64, FileDescriptor*> table; 

public : 

	// device manager
	int MountDevice(std::string path); 
	bool NewDevice(size_t size);
	bool unMountDevice();

	// system kernel 
	int openFS(BlockDevice* device); 
	bool closeFS(int fs_id); 

	// add FD
	int64 addFD(); 
	bool removeFD(int64 fd); 
};

