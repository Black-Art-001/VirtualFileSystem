#include "FileSystem.h"

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
		if (current != nullptr)
			delete current; 
		
		// update current 
		return path; 
	}
}

bool FileSystem::create_directory(FilePath path)
{
	
}

