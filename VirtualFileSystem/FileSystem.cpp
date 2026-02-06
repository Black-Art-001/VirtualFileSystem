#include "FileSystem.h"

bool FileSystem::dirGenerator(const std::string& path, inodeFlags permissions)
{
	PathResolver path_res(path, this);

	if (path_res.getStatus() == ResolverStatus::NOT_FOUND)
	{
		// check parent is directory or not 
		// 
		// get parent and chill
		PathComponent parent = path_res.get_parent(), chill = path_res.get_target();

		// set chill into parent list
		InodeManager parenIDM(*this, parent.id);
		DirectoryManager parentDM(parent.id, this);
		chill.id = InodeManager(*this, inodeType::DireMode).getInodeId(); // make new id ofr chill 
		parentDM.add(chill.name, chill.id); // add to parent 
		// update path resolve with new component ! 
		// 
		return true;
	}
	return false;
}

bool FileSystem::cd(const std::string& path)
{
	PathResolver pathList(path, this); 

	if (pathList.getStatus() != ResolverStatus::SUCCESS)
		return false; // current directory does not exist 

	pathList.unpinPath(cwd_dentry); // unpin old path 
	// update current working directory 
	cwd_dentry = pathList.get_target_dentry(); 
	pathList.pinPath(cwd_dentry); // pin new path
	
	return true;
}

bool FileSystem::mkdir(const std::string& path, inodeFlags permissions)
{
	
}


inline bool FileSystem::transfer_ownership(PathComponent& oldParent, PathComponent& newParent, PathComponent target , std::string new_name)
{
	// make dir manager of old and new 
	DirectoryManager oldPar(oldParent.id, this) , newPar(newParent.id , this) ;

	// remove from old 
	oldPar.remove(target.name); 
	// add to new with new name 
	newPar.add(new_name, target.id); 
}
