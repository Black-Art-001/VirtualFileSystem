#include "FileSystem.h"

bool FileSystem::create_directory(FilePath path, inodeID id)
{
	// allocate inode 
	// set default data in inode 
	// add it to current data 
	// add new dir to inode through DirManager 
}

FilePath FileSystem::current_path(FilePath path)
{
	if (path.size() == 0) //  
	{
		return currentPath; 
	}
	else
	{
		currentPath = path; 
		// resolve path 
		inodeID inode_id = pathResolver->convert(path); 
		if (currentDir != nullptr)
			delete currentDir; 
		
		// update current 
		currentDir = new InodeManager(inode_id); 
		currentInode = inode_id;
		return path; 
	}
}

bool FileSystem::create_directory(FilePath path)
{
	// create directory depends current inode 
}

bool FileSystem::create_directories(FilePath path)
{
	// splite new directories 
	// loop on new directories   
		// create directory depends current inode 
}

bool FileSystem::remove(FilePath path)
{
	
}

