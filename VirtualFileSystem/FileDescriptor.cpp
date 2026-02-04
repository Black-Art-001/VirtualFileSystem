#include "FileDescriptor.h"
#include <memory>
#include <algorithm>

#include "errors.h"


size_t FileDescriptor::tell()
{
	return cursor;
}

void FileDescriptor::seek(int64 offset, position mode)
{
	int64 end = inode->getSize(); 
	switch (mode)
	{
	case position::Beginning : 
		cursor = offset; 
		break; 
	case position::Current : 
		if (offset == 0)
			return; // do nothing 
		cursor += offset;
		break; 
	case position::End : 
		cursor = end + offset;
		break; 

	default : 
		throw std::invalid_argument("mode should be valid postion"); 
	}
	if (cursor_pos < 0) cursor_pos = 0;
	else if (cursor_pos > end) cursor_pos = end; 
}

size_t FileDescriptor::read(byte* buffer, size_t len)
{
	// base information 
	int64 file_size = inode->getSize();
	size_t totalRead = 0;
	size_t index = findSectorIndex(cursor);
	int64 offset = findOffset(cursor); 

	// invalidator !
	len = std::min(file_size - cursor, len);
	// read loop 
	while (totalRead < len)
	{
		// call page 
		SectorID sector_id = inode->getSector(index++);
		CachePage* page = bufferCache.GetPage(sector_id);

		// find size 
		size_t copy_size = std::min(sector_size - offset, len - totalRead);
		// copy data from cache to buffer 
		memcpy(buffer + totalRead, page->data + offset, copy_size);

		// update 
		totalRead += copy_size;
		offset = 0;
		page.unpin(); // upin page 
	}
	cursor += totalRead; // pointer move froward depend on how much we write ! 
	return totalRead;
}

size_t FileDescriptor::write(byte* buffer, size_t len)
{
	// base information 
	int64 maxfile_size = ceil(inode->getSize() / static_cast<double>(sector_size)) * sector_size; 
	size_t totalWrite = 0;
	size_t index = findSectorIndex(cursor); 
	uint64 offset = findOffset(cursor); 

	size_t normal_write = std::min(maxfile_size - cursor, len); 
	size_t out_write = len - normal_write; 
	
	while (normal_write > totalWrite)
	{
		// call page 
		SectorID sector_id = inode->getSector(index++);
		CachePage* page = bufferCache.GetPage(sector_id);

		// find size 
		size_t copy_size = std::min(sector_size - offset, len - totalWrite);
		// copy data from buffer to cache
		memcpy(page->data + offset, buffer + totalWrite, copy_size); 

		// update 
		totalWrite += copy_size;
		offset = 0;
		page->makeDirty(); 
		page.unpin(); // upin page 
	}

	while (out_write + normal_write > totalWrite)
	{
		SectorID sector_id = mapManager.alloc(inode_id); 
		CachePage* page = bufferCache.GetPage(sector_id); 

		// find size 
		size_t copy_size = std::min(sector_size, len - totalWrite); 
		// copy data from buffer to cache
		memcpy(page->data + offset, buffer + totalWrite, copy_size);

		// update
		totalWrite += copy_size;
		inode->appendSector(sector_id); 
		page->makeDirty();
		page.unpin(); // upin page 
	}
	cursor += totalWrite; 
	return totalWrite; 
}

size_t FileDescriptor::truncate()
{
	size_t totalSector = inode->getCursor().sector; 
	for(size_t index ; index < totalSector ; index++)
	{
		mapManager.free(inode->getSector(index)); // free memory ! 
	}
	inode->clear(); 
}