#pragma once

#include "types.h" 

class InodeManager {
public : 
	InodeManager(inodeID inode_id) {};
	inodeID getSector(inodeID IDs) { return 0; }
	size_t getSize() { return 0; }
	void append(SectorID sector_id) { return; }
};