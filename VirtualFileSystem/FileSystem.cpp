#include "FileSystem.h"

// ======= internall progress ========

inline bool FileSystem::transfer_ownership(PathComponent& oldParent, PathComponent& newParent, PathComponent& oldChill, string& new_name)
{
	{
		// remove from old parent 
		DirectoryManager oldParDM(oldParent.id, this), newParDM(newParent.id, this);

		if (newParDM.exist(new_name))
			return false; // we can not move to new pace wit this name 

		oldParDM.remove(oldChill.name);

		// add to new directory manager 		 
		newParDM.add(new_name, oldChill.id);
	} // free memory of two DM obj 

	// update D entry cache 
	PathResolver::syncMove(oldParent.id, oldChill.name, newParent.id, new_name);
	return true;
}

bool FileSystem::dirGenerator(const std::string& path, inodeFlags permissions)
{
	PathResolver path_res(path, this);

	if (path_res.getStatus() == ResolverStatus::NOT_FOUND)
	{
		// get parent and chill
		PathComponent parent = path_res.get_parent(), chill = path_res.get_target();
		// set chill into parent list
		InodeManager parentIDM(*this, parent.id);
		DirectoryManager parentDM(parent.id, this);
		// check parent is directory or not 
		if (parentIDM.getType() == inodeType::FileMode)
			return false;

		InodeManager chillIDM(*this, inodeType::DireMode); // make new id ofr chill 
		chill.id = chillIDM.getInodeId();
		chillIDM.setPermission(permissions);

		parentDM.add(chill.name, chill.id); // add to parent 
		// update path resolve with new component ! 
		PathResolver::syncMakeNode(parent.id, chill.name, chill.id, NodeType::DIRECTORY);
		return true;
	}
	return false;
}

bool FileSystem::remove_Directory(inodeID target , string& name)
{
	// for removeing directory i should move on all elemets in directory !
	// open directory 
	DirectoryManager dirMan(target, this); 

	// move on all subdirectory 
	size_t total_index = dirMan.getTotalIndex(); 

	// try to delete them 
	// check index by index 
	for (size_t index = 0; index < total_index; index++)
	{
		// get all in bucket
		const auto& list = dirMan.bucketEntries(index); // get all name _ id
		// start delete operation 
		for(auto it : list)
		{
			// check is it directory or file 
			InodeManager inode(*this, it.id);
			// remove it 
			const inodeType& type = inode.getType(); 
			if (type == inodeType::FileMode)
			{
				remove_File(it.id, it.name); 
			}
			else if (type == inodeType::DireMode)
			{
				remove_Directory(it.id , it.name); 
			}

			// remvoe from parent ! 
			dirMan.remove(it.name); 
		}
	}
	// updoate d entry cache 
	 
	return removeEmptyDir(target , name); // delete  
}

bool FileSystem::removeEmptyDir(inodeID target, string& name)
{
	DirectoryManager dm(target, this); 
	if (dm.isEmpty())
	{
		InodeManager(*this, target).unlink();
		PathResolver::syncRemove(target, name);
		return true; 
	}
		return false; 
}

bool FileSystem::remove_File(inodeID target, string& name)
{

	InodeManager inode(*this, target); 
	if(inode.getType() == inodeType::FileMode)
	{
		inode.unlink();
		PathResolver::syncRemove(target, name);
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

std::string FileSystem::get_current_path()
{

	return PathResolver::getCurrentPath(this);
}

bool FileSystem::mkdir( std::string& path, inodeFlags permissions)
{
	if (this->dirGenerator(path, permissions) == false)
		return false;

	// weak vailidator 
	return (PathResolver(path, this).getStatus() == ResolverStatus::SUCCESS);
}

bool FileSystem::mkdirs( std::string& path, inodeFlags premisions)
{
	// it can make inf loop ? 
	while (this->dirGenerator(path, premisions));
}

bool FileSystem::rmdir(std::string& path)
{
	PathResolver res(path , this); 
	auto parent = res.get_parent(), chill = res.get_target();
	// validator 
	if (this->removeEmptyDir(chill.id, chill.name))
	{
		DirectoryManager dm(parent.id, this); 
		dm.remove(chill.name); 
		return true;
	}
	return false; 
}

bool FileSystem::rmall(std::string& path)
{
	PathResolver res(path, this);
	auto parent = res.get_parent(), chill = res.get_target();
	// validator 
	if (this->remove_Directory(chill.id, chill.name))
	{
		DirectoryManager dm(parent.id, this);
		dm.remove(chill.name);
		return true;
	}
	return false;
}

bool FileSystem::mklink(const std::string& src_path, const std::string dst_path)
{
	PathResolver src_res(src_path, this), dst_res(dst_path, this); 
	// open src and dst
	
	// get inode from src 

	// write in dst with different name 

	// update num link 
}

