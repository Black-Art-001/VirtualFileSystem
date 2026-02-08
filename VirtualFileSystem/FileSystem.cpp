#include "FileSystem.h"
#include <algorithm>

#include "SystemKernel.h"

// ======= inline utils ========
// check parent is directory or not
inline bool checkParent(PathResolver& res)
{
	return res.get_target_type() == NodeType::DIRECTORY;
}

inline bool checkTarget(PathResolver& res)
{
	return (res.get_target_type() == NodeType::DIRECTORY);
}
// ======= internall progress ========

void FileSystem::setup_root()
{
}

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

	if (path_res.getStatus() == ResolverStatus::NOT_FOUND and checkParent(path_res))
	{
		// get parent and chill
		PathComponent parent = path_res.get_parent(), chill = path_res.get_target();
		// set chill into parent list
		DirectoryManager parentDM(parent.id, this);
		// check parent is directory or not 

		InodeManager chillIDM(this, inodeType::DireMode); // make new id ofr chill 
		chill.id = chillIDM.getInodeId();
		chillIDM.setPermission(permissions);

		parentDM.add(chill.name, chill.id); // add to parent 
		// update path resolve with new component ! 
		PathResolver::syncMakeNode(parent.id, chill.name, chill.id, NodeType::DIRECTORY);
		return true;
	}
	return false;
}

bool FileSystem::remove_Directory(inodeID target, string& name)
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
		for (auto it : list)
		{
			// check is it directory or file 
			InodeManager inode(this, it.id);
			// remove it 
			const inodeType& type = inode.getType();
			if (type == inodeType::FileMode)
			{
				remove_File(it.id, it.name);
			}
			else if (type == inodeType::DireMode)
			{
				remove_Directory(it.id, it.name);
			}

			// remvoe from parent ! 
			dirMan.remove(it.name);
		}
	}
	// updoate d entry cache 

	return removeEmptyDir(target, name); // delete  
}

bool FileSystem::removeEmptyDir(inodeID target, string& name)
{
	DirectoryManager dm(target, this);
	if (dm.isEmpty())
	{
		InodeManager(this, target).unlink();
		PathResolver::syncRemove(target, name);
		return true;
	}
	return false;
}

bool FileSystem::remove_File(inodeID target, string& name)
{

	InodeManager inode(this, target);
	if (inode.getType() == inodeType::FileMode)
	{
		inode.unlink();
		PathResolver::syncRemove(target, name);
		return true;
	}
	return false;
}

size_t FileSystem::getSize(PathComponent com)
{
	InodeManager inode(this, com.id);
	if (inode.getType() == inodeType::FileMode)
		return inode.getSize();
	else if (inode.getType() == inodeType::DireMode)
	{
		DirectoryManager dm(com.id, this);
		size_t total_size = 0;
		size_t max = dm.getTotalIndex(); // for directory we can return number of files in directory
		for (size_t i = 0; i < max; i++)
		{
			const auto bucket = dm.bucketEntries(i);
			for (auto it : bucket)
			{
				total_size += getSize({ it.name, it.id });
			}
		}

	}
	else
		return 0; // for directory we can return 0 or we can return number of files in directory
}

FileSystem::FileSystem(BlockDevice* device)
{
	cache = new BufferCache(device);
	pointer_map = new PointerMapManager(*cache); 
	uint32 startFirstInodePage = (device->getSuperblock()->inodeStart) ? device->getSuperblock()->inodeStart : pointer_map->alloc(DEFAULT_ROOT_INODE_ID);

	page_mgr = new InodePageManager(cache, pointer_map, startFirstInodePage); 
	indirect_mgr = new IndirectBlockManager(*cache, *pointer_map); 
	PathResolver::initCache();
}

FileSystem::~FileSystem()
{
	if (not cache)
		delete cache; 
	if (not pointer_map)
		delete pointer_map; 
	if (not page_mgr)
		delete page_mgr; 
	if (not indirect_mgr)
		delete indirect_mgr; 
}

// ======= externall progress ========


bool FileSystem::cd(const std::string& path)
{
	PathResolver pathList(path, this);

	if (pathList.getStatus() != ResolverStatus::SUCCESS)
		return false; // current directory does not exist 

	pathList.unpinDentry(cwd_dentry); // unpin old path 
	// update current working directory 
	cwd_dentry = pathList.get_target_dentry();
	pathList.pinDentry(cwd_dentry); // pin new path

	return true;
}

std::string FileSystem::get_current_path()
{

	return PathResolver::getCurrentPath(this);
}

bool FileSystem::mkdir(std::string& path, inodeFlags permissions)
{
	if (this->dirGenerator(path, permissions) == false)
		return false;

	// weak vailidator 
	return (PathResolver(path, this).getStatus() == ResolverStatus::SUCCESS);
}

bool FileSystem::mkdirs(std::string& path, inodeFlags premisions)
{
	// it can make inf loop ? 
	while (this->dirGenerator(path, premisions));
	return true; 
}

bool FileSystem::rmdir(std::string& path)
{
	PathResolver res(path, this);
	auto parent = res.get_parent(), chill = res.get_target();
	// check parent is directory and target is directory too
	if (res.getStatus() == ResolverStatus::SUCCESS and checkTarget(res))
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
	// if parent is directory and target is directory too  
	if ((res.getStatus() == ResolverStatus::SUCCESS) and checkTarget(res))
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
	// both of them should be directory
	if ((src_res.getStatus() == ResolverStatus::SUCCESS) and checkParent(dst_res))
	{
		auto src_par = src_res.get_parent(), src_chi = src_res.get_target();
		auto dst_par = dst_res.get_parent(), dst_chi = dst_res.get_target();

		// open src and dst
		DirectoryManager src(src_par.id, this), dst(dst_par.id, this);

		// get inode from src 
		auto id = src.findInode(src_chi.name);
		if (id == NULL_SECTOR)
			return false;

		// write in dst with different name 

		dst.add(dst_chi.name, id);
		// update num link 

		InodeManager(this, id).link();
		// update d entry cache
		PathResolver::syncMakeNode(dst_par.id, dst_chi.name, id, src_res.get_target_type());
		return true;
	}
	return false;
}

std::vector<std::string> FileSystem::ls(const std::string& path)
{
	PathResolver res(path, this);
	if (res.getStatus() == ResolverStatus::SUCCESS)
	{
		if (checkTarget(res)) // if target is directory we can just return all name in directory manager
		{
			std::vector<std::string> result;
			DirectoryManager dm(res.get_target().id, this);
			size_t total_index = dm.getTotalIndex();
			for (size_t index = 0; index < total_index; index++)
			{
				const auto& list = dm.bucketEntries(index); // get all name _ id
				for (auto it : list)
				{
					result.push_back(it.name);
				}
			}
			return result;
		}
		else // if target is file we can return empty vector or we can return name of file 
			return std::vector<std::string>{ res.get_target().name };
	}

	return std::vector<std::string>();
}

bool FileSystem::touch(const std::string& path, inodeFlags permissions)
{
	PathResolver path_res(path, this);
	if ((path_res.getStatus() == ResolverStatus::SUCCESS))
	{
		auto parent = path_res.get_parent(), chill = path_res.get_target();
		DirectoryManager parentDM(parent.id, this);
		if (not parentDM.exist(chill.name)) // if target does not exist 
		{
			InodeManager chillIDM(this, inodeType::FileMode); // make new id ofr chill 
			chill.id = chillIDM.getInodeId();
			chillIDM.setPermission(permissions);
			parentDM.add(chill.name, chill.id); // add to parent 
			// update path resolve with new component ! 
			PathResolver::syncMakeNode(parent.id, chill.name, chill.id, NodeType::FILE);
			return true;
		}
	}
	return false;
}

bool FileSystem::unlink(const std::string& path)
{
	PathResolver path_res(path, this);
	if ((path_res.getStatus() == ResolverStatus::SUCCESS))
	{
		auto parent = path_res.get_parent(), chill = path_res.get_target();
		DirectoryManager parentDM(parent.id, this);
		if (parentDM.exist(chill.name)) // if target exist 
		{
			InodeManager chillIDM(this, chill.id);
			if (chillIDM.getType() == inodeType::FileMode)
			{
				chillIDM.unlink(); // unlink from inode manager 
				parentDM.remove(chill.name); // remove from parent directory manager 
				PathResolver::syncRemove(parent.id, chill.name); // update dentry cache 
				return true;
			}
		}
	}
	return false;
}

bool FileSystem::rename(const std::string& old_path, const std::string& new_path)
{
	PathResolver old_res(old_path, this), new_res(new_path, this);
	auto old_par = old_res.get_parent(), old_chi = old_res.get_target();
	auto new_par = new_res.get_parent(), new_chi = new_res.get_target();

	if ((old_res.getStatus() == ResolverStatus::SUCCESS))
		if (new_par.id == old_par.id) // if they are in same directory we can just change name 
			if (transfer_ownership(old_par, new_par, old_chi, new_chi.name))
				return true;
	return false;
}

bool FileSystem::move(const std::string& old_path, const std::string& new_path)
{
	PathResolver old_res(old_path, this), new_res(new_path, this);
	if ((old_res.getStatus() == ResolverStatus::SUCCESS)
		and (new_res.getStatus() == ResolverStatus::SUCCESS))
	{
		auto old_par = old_res.get_parent(), old_chi = old_res.get_target();
		auto new_par = new_res.get_parent(), new_chi = new_res.get_target();

		// now we can move to different directory
		if (old_par.id != new_par.id) // they are not in same directory
			if (transfer_ownership(old_par, new_par, old_chi, new_chi.name))
				return true;
	}
	return false;
}

bool FileSystem::copy(const std::string& dst, const std::string& src)
{
	// for copy we can just read from src and write to dst
	PathResolver src_res(src, this), dst_res(dst, this);
	if (src_res.getStatus() == ResolverStatus::SUCCESS
		and dst_res.getStatus() == ResolverStatus::SUCCESS // both are directory
		and src_res.get_target_type() == NodeType::FILE)
	{
		auto src_par = src_res.get_parent(), src_chi = src_res.get_target();
		auto dst_par = dst_res.get_parent(), dst_chi = dst_res.get_target();
		// open src and dst
		DirectoryManager srcDM(src_par.id, this), dstDM(dst_par.id, this);
		if (dstDM.exist(dst_chi.name))
			return false; // if dst exist we can not copy to dst
		// get inode from src 
		InodeManager dstInode(this, inodeType::FileMode); // make new id ofr chill
		dst_chi.id = dstInode.getInodeId(); // get new id for dst
		// add to dst directory manager
		dstDM.add(dst_chi.name, dst_chi.id);

		// copy data from src to dst operation
		FileDescriptor srcFD(src_chi.id, this), dstFD(dst_chi.id, this);

		srcFD.seek(0, position::Beginning); // set cursor to beginning
		dstFD.truncate();
		byte* buf = new byte[DEFAULT_CACHE_SIZE]{};

		size_t read_bytes = 0;
		while (true)
		{
			read_bytes = srcFD.read(buf, DEFAULT_CACHE_SIZE);
			if (read_bytes == 0)
				break; // EOF
			dstFD.write(buf, read_bytes);

		}
		delete[] buf;
		// update d entry cache
		PathResolver::syncMakeNode(dst_par.id, dst_chi.name, dstInode.getInodeId(), src_res.get_target_type());
		return true;
	}
	return false;
}

int FileSystem::open(const std::string& path, inodeFlags mode, bool rootAccess)
{
	PathResolver res(path, this); 
	if (res.getStatus() == ResolverStatus::SUCCESS)
	{
		if (res.get_target_type() == NodeType::FILE)
		{
			InodeManager inode(this, res.get_target().id);
			if (rootAccess or inode.hasPermission(mode))
			return SystemKernel::addFD(new FileDescriptor(res.get_target().id, this)); 
		}
	}
	return -1;
}

bool FileSystem::close(int fd)
{
	return SystemKernel::closeFS(fd); 
}

FileDescriptor* FileSystem::get_fd_object(int fd)
{
	return SystemKernel::getFD(fd);
}

bool FileSystem::exists(const std::string& path)
{
	PathResolver res(path, this);
	if (res.getStatus() == ResolverStatus::SUCCESS)
	{
		if (checkTarget(res))
			return true; // if target is directory we can just check exist in directory manager
		else // if target is file we can check exist in directory manager of parent
		{
			DirectoryManager dm(res.get_parent().id, this);
			return dm.exist(res.get_target().name);
		}
	}
	return false;
}

uint64 FileSystem::get_size(const std::string& path)
{
	PathResolver res(path, this);
	// true validator == parent are directory and path exist ! 
	if (res.getStatus() == ResolverStatus::SUCCESS)
	{
		return getSize(res.get_target());
	}
	return 0;
}

bool FileSystem::is_dir(const std::string& path)
{
	return PathResolver(path, this).get_target_type() == NodeType::DIRECTORY;
}

bool FileSystem::set_perms(const std::string& path, inodeFlags perms)
{
	PathResolver res(path, this); 
	if (res.getStatus() == ResolverStatus::SUCCESS); 
	{
		InodeManager inode(this, res.get_target().id);
		inode.setPermission(perms);
		return true;
	}
	return false;

}

inodeType FileSystem::get_node_type(const inodeID id)
{
	return InodeManager(this, id).getType();
}
