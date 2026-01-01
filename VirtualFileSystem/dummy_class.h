#pragma once

#include "types.h" 

class inodeManager
{
public : 
	inline size_t totalSector() { return 0; }
	SectorID getSector(int index) { return 0; }
	void setSector(SectorID new_sector) {return;}
	Cursor getCursor() { return Cursor(0, 0);  }
	inline size_t getSize() { return 0; }
	void clear() {}
};