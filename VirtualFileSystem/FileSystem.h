#pragma once

#include "BufferCache.h"
#include "InodeManager.h"
#include "FileDescriptor.h"
#include "dummy_class.h"
#include "DirectoryManager.h"

#include <unordered_map>

class FileSystem
{
private : 
	BufferCache* bufferCache;
	FilePath currentPath; 
	inodeID currentInode; 
	InodeManager *currentDir; 
	PathResolver* pathResolver; 
	DirectoryManager* DirManager;
	std::unordered_map< inodeID , InodeManager*> inodes; 
	std::unordered_map< uint32, FileDescriptor*> files; 

	bool create_directory(FilePath path, inodeID id); 
public : 
	FileSystem(); 

	FilePath current_path(FilePath path); 
	
	bool create_directory(FilePath path);
	bool create_directories(FilePath path); 
	bool remove(FilePath path);
	bool remove_all(FilePath path);

	bool copy(FilePath src, FilePath dst); 
	bool rename(FilePath path); 

	bool exists(FilePath path);
	bool is_empty(FilePath path); 
	bool is_directory(FilePath path); 

	bool create_hard_link(FilePath path); 
	
};

