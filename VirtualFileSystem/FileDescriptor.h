#pragma once

#include "types.h"
#include "BufferCache.h"
#include "PointerMapManager.h"
#include "dummy_class.h"

enum class position {
	Beginning , 
	Current , 
	End
};

class FileDescriptor
{
private : 
	inodeID inode_id;
	BufferCache& bufferCache; 
	PointerMapManager& mapManager;
	inodeManager* inode;
	uint64 cursor;
	size_t sector_size; 
	/// access mode 
	

	inline SectorID findSectorIndex(int64 cursor)
	{
		return cursor / sector_size; 
	}
	inline int64 findOffset(int64 cursor)
	{
		return cursor / sector_size; 
	}

public : 
	FileDescriptor(inodeID inode_id);
	~FileDescriptor(); 
	size_t tell();
	void seek(int64 pos , position mode = position::Current);
	size_t read(byte* buffer , size_t len);
	size_t write(byte* buffer , size_t len);
	size_t truncate(); 

};

