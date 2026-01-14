#include "DirectoryManager.h"
#include <memory>
#include <algorithm>


#define BASE_COUNTER_SIZE 2

inodeID DirectoryManager::find(const char* name, uint32 name_size, uint32 group_id)
{
	DirDataBlock block(this, group_id); 
	if (block.tableSize < 1)
		return false; // we can not find anything !  

	const byte* data = reinterpret_cast<const byte*>(name); 
	block.readBlock(); 
	uint32 firstIndex = 0, lastIndex = block.tableSize - 1; 
	uint32 middle;
	while (firstIndex <= lastIndex)
	{
		middle = (firstIndex + lastIndex) / 2;
		uint32 currentIndex = block.table_index(middle); 
		uint32 nextIndex = block.table_index(middle + 1);
		int result = block.compare(data, currentIndex, name_size, nextIndex - currentIndex);

		if (result == 0)
			return block.getInode(nextIndex - sizeof(inodeID)); // we send next index instead of current + name size  
		else if (result == 1) // name > middle data
			firstIndex = middle + 1;
		else // name < middle data 
			lastIndex = middle - 1; 
	}
	// we can not found this name 
	return NULL_INODE; 
}

bool DirectoryManager::exist(string name)
{
	return findInode(name);
}

inodeID DirectoryManager::findInode(string name)
{
	// get filled size 
	uint32 totalBlock = std::floor(inodeManager->getSize() / static_cast<double>(groupSize));
	for (inodeID gr = 0; gr < totalBlock; gr++)
	{
		inodeID result = find(name.c_str(), name.length(), gr);
		if (result != NULL_INODE)
			return result;
	}
	return NULL_INODE;
}

bool DirectoryManager::add(string name, inodeID inode_id)
{
	Counter nameSize = name.length(); 
	uint32 totalLength = nameSize + sizeof(inodeID); // struct : [name][inode]
	uint32 totalGroup = inodeManager->getSize(); 
	
	for (uint32 i = 0; i < totalGroup; i++)
	{
		DirDataBlock block(this, i);
		if (block.freeSpace >= totalLength + sizeof(Counter))
		{
			block.readBlock(); 
			uint32 firstIndex = 0, lastIndex = block.tableSize - 1;
			uint32 middle;
			int result; 
			while (firstIndex <= lastIndex)
			{
				middle = (firstIndex + lastIndex) / 2;
				uint32 currentIndex = block.table_index(middle);
				uint32 nextIndex = block.table_index(middle + 1);

				result = block.compare(reinterpret_cast<const byte*>(name.c_str()),
					currentIndex, name.size(), nextIndex - currentIndex); 
				if (result == 1)
					firstIndex = middle + 1;
				else if (result == -1)
					lastIndex = middle - 1;
				else
					throw std::runtime_error("this name existed"); 
			}
			if (result == 1) 
				middle++;	// we should skipe frome middle and shift from next data  
			uint32 index = block.table_index(middle);
			block.Forward_shift(index, block.table_index(block.tableSize - 1), name.size() + sizeof(Counter)); 
			block.Forward_shift(middle, index, sizeof(Counter)); 
			index += 2; 

			block.overwrite(reinterpret_cast<const byte*>(&nameSize),
				middle, sizeof(Counter)); 
			block.overwrite(reinterpret_cast<const byte*>(&nameSize),
				index, name.size()); 

			// update free size 
			Counter temp = block.freeSpace + name.size() + sizeof(Counter);
			block.overwrite(reinterpret_cast<const byte*>(&temp),
				0, sizeof(Counter)); 
			block.updateTable(middle, name.size()); // upodate all index 
			return true; // we insert data seccessfully  
		}
	}
	// we need to allocate table 

	CachePage** pages = new CachePage * [groupSize] {nullptr}; 
	DirDataBlock block(this); 
	for (uint32 i = 0; i < groupSize; i++)
	{
		SectorID SID = mapManager->alloc(inode_id); 
		pages[i] = cache->GetPage(SID); 
	}
	block.pages = pages; 
	pages = nullptr; 
	// [free space][table : index 1 , index 2(endof index)][data , inodeID]
	Counter usedSpace =  sizeof(Counter)*3 + name.size() + sizeof(inodeID); 
	Counter firstData[4]; 
	firstData[0] = groupSize * cache->getSectorSize() - usedSpace; // calculate free space
	firstData[1] = sizeof(Counter)*2; // tableSize = pointer to data , pointer to end 
	firstData[2] = sizeof(Counter) * 4; // point to start of data 
	firstData[3] = usedSpace;
	block.overwrite(reinterpret_cast<const byte*>(firstData), 0, sizeof(Counter) * 4); // write free space , table size ,table
	block.overwrite(reinterpret_cast<const byte*>(name.c_str()),
		sizeof(Counter) * 4, name.size());  // write name
	block.overwrite(reinterpret_cast<const byte*>(&inode_id),
		sizeof(Counter) * 4 + name.size(), sizeof(inodeID)); // write inode id
}

bool DirectoryManager::remove(string name)
{
	Counter nameSize = name.length();
	uint32 totalLength = nameSize + sizeof(inodeID); // struct : [name][inode]
	uint32 totalGroup = inodeManager->getSize();

	for (uint32 i = 0; i < totalGroup; i++)
	{
		DirDataBlock block(this, i);
		if (block.freeSpace >= totalLength + sizeof(Counter))
		{
			block.readBlock();
			uint32 firstIndex = 0, lastIndex = block.tableSize - 1;
			uint32 middle;
			int result;
			while (firstIndex <= lastIndex)
			{
				middle = (firstIndex + lastIndex) / 2;
				uint32 currentIndex = block.table_index(middle);
				uint32 nextIndex = block.table_index(middle + 1);

				result = block.compare(reinterpret_cast<const byte*>(name.c_str()),
					currentIndex, name.size(), nextIndex - currentIndex);
				if (result == 1)
					firstIndex = middle + 1;
				else if (result == -1)
					lastIndex = middle - 1;
				else
				{
					uint32 index = block.table_index(middle);
					uint32 next_index = block.table_index(middle + 1); 
					uint32 end = block.table_index(middle); 
					block.backward_shift(middle, block.table_index(middle), sizeof(Counter)); 
					block.backward_shift(index, end, next_index - index + sizeof(Counter)); // name size + inode size + counter index
					Counter baseData[2] = {
						block.freeSpace - (next_index - index + sizeof(Counter))
							, block.tableSize - sizeof(Counter)};
					block.overwrite(reinterpret_cast<const byte*>(baseData), 0, sizeof(Counter) * 2); // write base data 
					return true; 
				}
			}
			
		}
	}
	return false;
}

DirTable DirectoryManager::list(size_t size)
{
	uint32 total = inodeManager->getSize() / groupSize;
	if (size > total)
		size = total; 
	uint32 total_table_size = 0; 
	uint32 total_data_size = 0; 

	for (uint32 i = 0; i < size; i++)
	{
		DirDataBlock block(this, i); 
		total_data_size += (groupSize * cache->getSectorSize() - block.freeSpace);
		total_table_size += block.tableSize;
	}
	if (total_table_size == 0)
	{
		return DirTable(nullptr, nullptr, 0); 
	}
	
	Counter* table = new Counter[total_table_size / sizeof(Counter)]{};
	byte* data = new byte[total_data_size]; 

	size_t read_data = 0, read_table = 0; 

	table[0] = 0; 

	for (uint32 i = 0; i < size; i++)
	{
		DirDataBlock block(this, i); 
		block.readBlock();
		size_t dataSize = block.table_index(block.tableSize - 1) - block.table_index(0);
		
		// make a table 
		if (read_table == 0)
		{
			for (int i = 0; i < block.tableSize; i++)
			{
				table[i] = static_cast<uint32>(*(block.pages[i / sectorSize]->data + i % sectorSize));
			}
		}
		else 
		{
			for (int i = 0; i < block.tableSize; i++)
			{
				table[i + read_table] = table[read_table - 1] +
					static_cast<uint32>(*(block.pages[i / sectorSize]->data + i % sectorSize));
			}
		}
		// copy data
		uint32 startPoint = block.table_index(0); 
		for (int i = 0; i < dataSize; i++)
		{
			data[i + read_data] = *(block.pages[startPoint + i / sectorSize]->data + i % sectorSize);
		}

		read_table += block.tableSize; 
		read_data += dataSize;
	}

	return DirTable(table, data, total_table_size); 
}
