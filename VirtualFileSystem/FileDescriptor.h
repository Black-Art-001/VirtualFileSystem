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
	inodeID inode;
	BufferCache& bufferCache; 
	PointerMapManager& mapManager;
	inodeManager* inode; 
	Cursor cursor;
	/// access mode 

public : 
	FileDescriptor(inodeID inode_id);
	~FileDescriptor(); 
	size_t tell();
	void seek(size_t pos , position mode);
	size_t read(byte* buffer , size_t len);
	size_t write(byte* buffer , size_t len);
};

