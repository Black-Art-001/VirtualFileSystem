#pragma once


#include "types.h" 
#include "BufferCache.h" 
#include "PointerMapManager.h"
#include "dummy_class.h" 
#include "errors.h"

#include <string>

using std::string; 

struct DirTable
{
	Counter* Table; // table of index
	byte* Index; // data 
	uint32 table_size; 
	void free()
	{
		if (Table != nullptr)
			delete[] Table;
		if (Index != nullptr)
			delete[] Index; 
	}
};

class DirectoryManager
{
private : 
	InodeManager* inodeManager; 
	PointerMapManager* mapManager;
	BufferCache* cache; 
	const uint32 groupSize;
	uint32 sectorSize;
	const size_t Blocks_size; // size of block in byte
 
	inodeID find(const char* name , uint32 name_size, uint32 group_id); 


	struct DirDataBlock {
		Counter freeSpace, tableSize; 
		inodeID inode_id; 
		CachePage *header_page , **pages; 
		DirectoryManager* parent; 
		uint32 sectorSize; 
		uint32 size;
		DirDataBlock(DirectoryManager* parent) :
			pages(nullptr), header_page(nullptr), parent(parent)
		{
			sectorSize = parent->cache->getSectorSize();
		}
		DirDataBlock(DirectoryManager* parent_ , inodeID index_group) : 
			DirDataBlock(parent_) , inode_id(index_group)
		{
			// read first page 
			inodeID first_ID = parent_->inodeManager->getSector(index_group); 
			check_if(first_ID == NULL_INODE , std::runtime_error ,
				"Faild to find ! Inode ID not fond in inode\nrequire inode does not exist");
			header_page = parent_->cache->GetPage(first_ID);
			memcpy(&freeSpace, header_page->data, sizeof(Counter)); 
			memcpy(&tableSize, header_page->data + sizeof(Counter), sizeof(Counter));
		}

		void readBlock()
		{
			pages = new CachePage * [parent->groupSize]; 
			pages[0] = header_page; 
			for (uint32 i = 1; i < parent->groupSize; i++)
			{
				inodeID ID = parent->inodeManager->getSector(inode_id + i);
				check_if(ID == NULL_INODE, std::runtime_error,
					"Faild to find ! Inode ID not fond in inode\nrequire inode does not exist");
				pages[i] = parent->cache->GetPage(ID);
			}
		}

		uint32 table_index(Counter index)
		{
			check_if(index > tableSize, std::out_of_range, "index out of range for this table"); 
			uint32 location = (index + sizeof(Counter) * 2); 
			uint32 sector = location /parent->cache->getSectorSize()
				, offset = location % parent->cache->getSectorSize();
			return reinterpret_cast<Counter>(pages[sector]->data + offset); 
		}

		int compare(const byte* buffer, uint32 index, uint32 bufferSize , uint32 indexSize)
		{
			uint32 size = std::min(bufferSize, indexSize); 
			for (uint32 i = 0; i < size; i++)
			{
				byte a = *(pages[(index + i) / sectorSize]->data + (index + i % sectorSize));
				if (a > *(buffer + i))
					return -1;
				else if (a < *(buffer + i))
					return 1; 
			}
			if (indexSize < bufferSize)
				return 1;
			else if (indexSize == bufferSize)
				return 0; 
			return -1; 
		}

		void Forward_shift(uint32 first_index, uint32 last_index , uint32 shift_size)
		{
			check_if(first_index < sizeof(Counter) * 2, std::invalid_argument, "we can not move base data"); 
			check_if(shift_size > freeSpace, std::runtime_error, "we do not have enough space to shif table data "); 
			
			while (last_index >= first_index)
			{
				*(pages[(last_index + shift_size) / sectorSize]->data + (last_index + shift_size) % sectorSize) =
					*(pages[last_index / sectorSize]->data + last_index % sectorSize);
				last_index--; 
			}
			for (uint32 i = 0; i < std::ceil((shift_size + first_index % sectorSize) / sectorSize); i++)
			{
				pages[first_index/sectorSize + i]->makeDirty(); // mark all 
			}
		}

		void backward_shift(uint32 first_index, uint32 last_index, uint32 shift_size)
		{
			check_if(first_index < sizeof(Counter) * 2, std::invalid_argument, "we can not move base data");
			check_if(shift_size > freeSpace, std::runtime_error, "we do not have enough space to shif table data ");

			while (last_index >= first_index)
			{
				*(pages[(first_index - shift_size) / sectorSize]->data + (first_index - shift_size) % sectorSize) =
					*(pages[first_index / sectorSize]->data + first_index % sectorSize);
				first_index++;
			}
			for (uint32 i = 0; i < std::ceil((shift_size + first_index % sectorSize) / sectorSize); i++)
			{
				pages[last_index / sectorSize - i]->makeDirty(); // mark all 
			}
		}

		inodeID getInode(uint32 index)
		{
			uint32 temp = 0;
			for (uint32 i = 0; i < sizeof(inodeID); i++)
			{
				temp *= 8; 
				temp += static_cast<uint32>(*(pages[index / sectorSize]->data + index % sectorSize)); 
			}
			return temp;
		}

		void overwrite(const byte* buffer, uint32 index, uint32 size)
		{
			check_if(index < sizeof(Counter) * 2, std::invalid_argument, "we can not write on base data");
			for(uint32 i = 0 ; i < size ; i++)
			{
				*(pages[(index + i) / sectorSize]->data + (index + i) % sectorSize) = buffer[i];
			}
			for (uint32 i = 0; i < std::ceil((size + index%sectorSize) / sectorSize); i++)
			{
				pages[(index) / sectorSize + i]->makeDirty(); // mark all 
			}
		}

		void updateTable(uint32 table_index, uint32 total_size)
		{
			table_index += sizeof(Counter); // we add one data before new indexwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
			uint32 location = table_index + sizeof(Counter); // find true location 

			for (uint32 i = table_index; i < tableSize; i++)
			{
				Counter key = reinterpret_cast<Counter>(pages[location / sectorSize] + location % sectorSize) + total_size; 
				overwrite(reinterpret_cast<const byte*>(&key), i * sizeof(Counter), sizeof(Counter)); 
			}
		}

		~DirDataBlock() {
			if (pages != nullptr)
			{
				for (uint32 i = 0; i < parent->groupSize; i++)
					pages[i]->unpin(); 
				delete[] pages; 
			}
			else if (header_page != nullptr)
			{
				header_page->unpin(); 
			}
		}
	};

	friend struct  DirDataBlock;

public: 
	DirectoryManager(); 
	~DirectoryManager(); 
	bool exist(string name); 
	inodeID findInode(string name); 
	bool add(string name , inodeID inode_id); 
	bool remove(string name); 
	DirTable list(size_t size = 1);
	
};

