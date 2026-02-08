#pragma once

#include "BufferCache.h"
#include "PointerMapManager.h"
#include "InodeManager.h"
#include "FileSystem.h"
#include "types.h"

enum class position {
	Beginning,
	Current,
	End
};

class FileDescriptor
{
private : 
	BufferCache& bufferCache; 
	PointerMapManager& mapManager;
	inodeID inode_id = NULL_INODE; 
	InodeManager* inode = nullptr;
	uint64 cursor;
	size_t sector_size; 
	/// access mode 
	
	inline SectorID findSectorIndex(int64 cursor)
	{
		return cursor / sector_size; 
	}
	inline int64 findOffset(int64 cursor)
	{
		return cursor % sector_size; 
	}

public : 
	FileDescriptor(inodeID inode_id , FileSystem * fs);
	~FileDescriptor(); 
	uint64 getSize() { return inode->getSize(); }
	size_t tell();
	void seek(int64 pos , position mode = position::Current);
	size_t read(byte* buffer , size_t len);
	size_t write(byte* buffer , size_t len);
	size_t truncate(); 
	const InodeManager* const getInode() const noexcept { return inode; }
};

