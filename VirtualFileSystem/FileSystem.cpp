#include "FileSystem.h"

size_t FileSystem::copy(inodeID src, inodeID dst)
{
	if (src == NULL_INODE or dst == NULL_INODE)
		return false; 
	
	FileDescriptor srcFD(src, this), dstFD(dst, this); 
	
	srcFD.seek(0, position::End); 
	size_t size = srcFD.tell();
	srcFD.seek(0, position::Beginning); 

	size_t counter = 0 , len = bufferCache->getSectorSize(); 

	while (counter < size)
	{
		byte* buf = new byte[len];
		srcFD.read(buf, len); 
		dstFD.write(buf, len); 
		delete[]buf; 
	}
	return true; 
}

inodeID FileSystem::allocate(inodeID parent, std::string name, inodeType type)
{
	inodeID inode_id = pageManager->allocInode();
	InodeManager new_inode(inode_id , this);
	
	new_inode.setType(type);

	DirManager->add(name, inode_id);
	return inode_id;
}

bool FileSystem::unlink(inodeID inode_id, InodeManager& target_inode)
{
	target_inode.unlink();
	if (target_inode.getPermison() == inodeFlags::DeleteAccess)
	{
		return pageManager->freeInode(target_inode);
	}
	return true;
}

bool FileSystem::rmdir(inodeID inode_id, InodeManager& targetInode)
{
	DirectoryManager dM(inode_id, this);
	targetInode.unlink();

	// delete subdirectory operation 
	if (targetInode.getPermison() == inodeFlags::DeleteAccess)
	{
		size_t totoal_block = dM.getTotalroupBlock();
		for (size_t i = totoal_block - 1; i >= 0; i--)
		{
			DirTable dT = dM.getGroupBlock(i);
			for (size_t j = 0; j < dT.table_size; j++)
			{
				this->remove(dT.table[j].inode_id);
			}
		}
	}

	if (targetInode.getPermison() == inodeFlags::DeleteAccess)
		pageManager->freeInode(targetInode);

	return true;
}

inodeID FileSystem::remove(inodeID inode_id)
{
	InodeManager iM(inode_id, this);
	inodeID parentID = iM.getMetadata()->parentID;

	if (iM.getType() == inodeType::FileMode)
	{
		this->unlink(inode_id, iM);
	}
	else if (iM.getType() == inodeType::DireMode)
	{
		this->rmdir(inode_id, iM);
	}
	else
	{
		throw std::runtime_error("invalid type");
	}
	return parentID;
}

void FileSystem::Remove(inodeID inode_id)
{
	inodeID parentID = remove(inode_id);
	DirectoryManager dM(parentID, this);
	dM.remove(inode_id);
}

string FileSystem::current_path(string path)
{
	if (path.size() == 0)
		return path; 

	PathSplitList p(path); 
	inodeID id = pathResolver->getInode(p);
	if (id == NULL_INODE)
		throw std::invalid_argument("this path does not exist");
	currentInode = id;
	currentPath = path; 
	return path; 
}

bool FileSystem::create_directory(string path)
{
	PathSplitList sPath(path);

	auto pList = sPath.getList();

	auto it = pathResolver->getFirstNotExistingIterator(sPath); 
	
	inodeID parent = pathResolver->getInode(sPath, pathResolver->getFirstExistingIndex(sPath));
	
	allocate(parent , *it, inodeType::DireMode); 
}

bool FileSystem::create_directories(string path)
{
	PathSplitList sPath(path); 

	if (pathResolver->getInode(sPath) != NULL_INODE)
		throw std::invalid_argument("this pass exist"); 
	if (pathResolver->getFirstNotExistingIndex(sPath) == -1)
		throw std::invalid_argument("");

	size_t size = sPath.getList()->size() - pathResolver->getFirstNotExistingIndex(sPath); 
	while (size-- > 0)
	{
		uint32 index = pathResolver->getFirstExistingIndex(sPath); 
		if (index == -1)
			return false; 

		auto parent = pathResolver->getInode(sPath, index); 
		
		auto it = pathResolver->getFirstNotExistingIterator(sPath); 
		if (it == sPath.getList()->end())
			return false; 

		inodeID new_inode_id = pageManager->allocInode(); 

		DirectoryManager dirMan(parent, this); 

		dirMan.add(*it, new_inode_id); 
	}	
}

bool FileSystem::removeEmpty(string path)
{
	inodeID inode_id; // = get inode from pathresolver 
	InodeManager iM(inode_id, this);
	if (iM.getSize() == 0)
	{
		Remove(inode_id);
		return  true;
	}
	return false;
}

void FileSystem::remove_all(string path)
{
	inodeID inode_id; // = get inode from pathresolver 
	Remove(inode_id);
}

void FileSystem::move(string src, string dst , bool replace = false)
{
	PathSplitList srcPath(src) , dstPath(dst);
	inodeID src_id = pathResolver->getInode(srcPath); 
	inodeID dst_id = pathResolver->getInode(dstPath);
	if (src_id == NULL_INODE)
		throw std::invalid_argument("source path is invalid"); 
	if (dst_id == NULL_INODE)
		throw std::invalid_argument("destination path is invalid");

	auto target = pathResolver->getLastExistingIterator(srcPath); 

	InodeManager srcInode(src_id, this) , dstInode(dst_id ,this) ;

	if (dstInode.getType() == inodeType::FileMode)
		throw std::invalid_argument("we can move to file"); 

	inodeID src_parent = srcInode.getParentInode();

	DirectoryManager srcDirMan(src_parent, this), dstDirMan(dst_id, this);
	
	inodeID sameName = dstDirMan.findInode(*target); 

	if (sameName == NULL_INODE)
	{
		if (replace)
			dstDirMan.remove(sameName);
		else
			throw std::invalid_argument("source name reserved in destination"); 
	}

	// move from dst to src 
	dstDirMan.add(*target, src_id); 
	srcDirMan.remove(src_id); 
}

void FileSystem::copy(string src, string dst, bool replace)
{
	PathSplitList dstList(dst), srcList(src); 
	if (pathResolver->getInode(dstList) == NULL_INODE)
		throw std::runtime_error("we can not copy into not exist path"); 
	if (pathResolver->getInode(dstList) == NULL_INODE)
		throw std::runtime_error("we can not copy not existed data");

	inodeID src_id = pathResolver->getInode(srcList); 
	inodeID dst_id = pathResolver->getInode(dstList);

	InodeManager srcInode(src_id, this), dstInode(dst_id, this); 

	auto name = pathResolver->getLastExistingIterator(dstList); 

	if (dstInode.getType() != inodeType::DireMode)
		throw std::runtime_error("we can not copy into file"); 
	
	inodeID new_id = pageManager->allocInode();
	if(copy(src_id, new_id) == 0)
		throw std::runtime_error("copy failed");

	DirectoryManager dstDirMan(dst_id, this); 
	dstDirMan.add(*name, new_id);
}

bool FileSystem::rename(string path, string newName)
{
	PathSplitList pathList(path); 

	if (pathResolver->getInode(pathList) == NULL_INODE)
		return false;

	inodeID parent = pathResolver->getInode(pathList, pathList.getList()->size() - 2); 
	auto it = pathResolver->getLastExistingIterator(pathList); 
	it--; // point ot parent name 

	DirectoryManager dirMan(parent, this); 

	inodeID temp = dirMan.findInode(*it); 
	dirMan.remove(temp); 
	dirMan.add(*(it++), temp); // add inode with new id 
	return true;
}

bool FileSystem::exists(string path)
{
	PathSplitList pathList(path); 

	inodeID inode_id = pathResolver->getInode(pathList);

	return inode_id; 
}

bool FileSystem::is_empty(string path)
{
	PathSplitList pathList(path);

	inodeID inode_id = pathResolver->getInode(pathList);

	if (inode_id == NULL_INODE)
		return false;
	
	InodeManager inodeMan(inode_id, this); 

	if (inodeMan.getSize() == 0)
		return true; 
	return false; 
}

bool FileSystem::is_directory(string path)
{
	PathSplitList pathList(path);

	inodeID inode_id = pathResolver->getInode(pathList);

	if (inode_id == NULL_INODE)
		throw std::runtime_error("this path does not exist"); 

	InodeManager inodeMan(inode_id, this);

	if (inodeMan.getType() == inodeType::DireMode)
		return true; 
	return false;
}

void FileSystem::create_hard_link(string target, string new_obj)
{
	PathSplitList targetList(target), newObjList(new_obj);

	inodeID newObjID = pathResolver->getInode(newObjList);
	inodeID targetID = pathResolver->getInode(targetList);

	if (newObjID != NULL_INODE)
		throw std::runtime_error("this address reserved");
	if (targetID == NULL_INODE)
		throw std::runtime_error("we can not make an hard link not exist object");

	inodeID parent = pathResolver->getInode(newObjList , pathResolver->getFirstExistingIndex(newObjList));
	InodeManager targetInode(targetID, this);

	DirectoryManager dirMan(parent , this);
	dirMan.add(*(pathResolver->getFirstNotExistingIterator(newObjList)), targetID);
	targetInode.getMutableMetadata()->linkCount++;
}

FileDescriptor* FileSystem::open(string path)
{
	PathSplitList pathList(path); 
	inodeID inode_id = pathResolver->getInode(pathList); 
	
	if (inode_id == NULL_INODE)
		throw std::invalid_argument("this path does not exist !"); 

	auto it = fileHandler.find(inode_id);
	if (it == fileHandler.end())
	{
		if (fileHandler.size() == MAX_FD_SIZE)
			throw std::runtime_error("we do not have more free file handler"); 

		FileDescriptor* fd = new FileDescriptor(inode_id, this); 
		fileHandler[inode_id] = fd; 
		return fd;
	}
 	else
		return it->second; 
}
