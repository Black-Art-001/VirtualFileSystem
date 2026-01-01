#include "FileDescriptor.h"
#include <memory>

size_t FileDescriptor::tell()
{
	 return cursor.sector * bufferCache.getSectorSize() + cursor.offset ;
}

void FileDescriptor::seek(size_t pos, position mode)
{

}


size_t FileDescriptor::read(byte* buffer, size_t len)
{
	size_t total = 0;

	size_t sector_size = bufferCache.getSectorSize();
	size_t file_size = inode->getSize();
	size_t current_pos = cursor.sector * sector_size + cursor.offset;

	if (current_pos > file_size) return 0;
	if (current_pos + len > file_size) len = file_size - current_pos;
	while (total <= len)
	{
		size_t copy_size = sector_size - cursor.offset;
		
		//copy_size = min(copy_size , )

	}
}

size_t FileDescriptor::write(byte* buffer, size_t len)
{
	size_t reminder = len; 
	while (reminder > 0)
	{
		// find sector 
		// call page throw sector 
		// allocate if need more space 
		// write on page 
		// update cursor 
		// update reminder
		// update counter 
	}
}

