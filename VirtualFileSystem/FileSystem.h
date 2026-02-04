#pragma once

#define MAX_FD_SIZE 1024

#include "BufferCache.h"
#include "InodeManager.h"
#include "InodePageManager.h"
#include "FileDescriptor.h"
#include "DirectoryManager.h"
#include "PathResolver.h"
#include "PathSplitList.h"
#include <unordered_map>

#include <list>

class FileSystem
{
private : 
	string currentPath; 
	inodeID currentInode; 
	PathResolver* pathResolver;

	InodeManager *currentDir; 
	DirectoryManager* DirManager;
	InodePageManager* pageManager; 

	BufferCache* bufferCache;
	PointerMapManager* mapManager; 

	std::unordered_map< inodeID , InodeManager*> inodes; 
	std::unordered_map< uint32, FileDescriptor*> fileHandler; 

	size_t copy(inodeID src, inodeID dst);

	inodeID allocate(inodeID parent , std::string name , inodeType type);
	// unlink file 
	bool unlink(inodeID inode_id , InodeManager & inodeManager); 
	// remove directory 
	bool rmdir(inodeID inode_id, InodeManager& tInode);

	inodeID remove(inodeID inode_id); 
	void Remove(inodeID inode_id); 


public : 
	FileSystem(); 

	string current_path(string path); 
	
	bool create_directory(string path);
	bool create_directories(string path); 
	bool removeEmpty(string path);
	void remove_all(string path);

	void move(string src, string dst , bool replace = false); 
	void copy(string src, string dst , bool replace = false);
	bool rename(string path , string newName); 

	bool exists(string path);
	bool is_empty(string path); 
	bool is_directory(string path); 

	void create_hard_link(string target , string new_obj); 
	
	FileDescriptor* open(string path); 
	
	PointerMapManager* getPointerMapManager();
	BufferCache* getBufferCache();
};

