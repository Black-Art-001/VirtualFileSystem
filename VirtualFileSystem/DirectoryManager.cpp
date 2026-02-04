#include "DirectoryManager.h"
#include <exception>
#include <algorithm>

class DirectoryManager::TABLE_NODE
{
private:
	static const size_t HEADER_SIZE = sizeof(SectorID) + sizeof(uint16);
	vector<INDEX> entries;
	bool changed = false;
	SectorID next = NULL_SECTOR;
	SectorID prev = NULL_SECTOR;
	CachePage* page = nullptr;
	uint16 free_size = 0;
	DirectoryManager* dm;

	uint16& freeSize(byte* data)
	{
		return *reinterpret_cast<uint16*>(data + sizeof(SectorID));
	}
	SectorID& nextNode(byte* data)
	{
		return *reinterpret_cast<SectorID*>(data);
	}

public:
	static size_t headerSize()
	{
		return HEADER_SIZE;
	}
	TABLE_NODE(SectorID sector_id, DirectoryManager* dm)
	{
		this->dm = dm;
		page = dm->cache->GetPage(sector_id);
		if (page == nullptr)
			throw std::runtime_error("Failed to read directory table from disk.");

		free_size = freeSize(page->data);
		next = nextNode(page->data);

	}

	vector<INDEX>& getEntries()
	{
		byte* data = page->data;
		if (data == nullptr)
			throw std::runtime_error("Failed to read directory table from disk.");
		
		entries.clear();

		size_t offset = sizeof(SectorID) + sizeof(uint16); // skip next node and free size
		for (uint32 read_bytes = offset; read_bytes < dm->sectorSize - free_size; )
		{
			uint16 data_len = *reinterpret_cast<uint16*>(data + read_bytes);
			if (data_len < sizeof(SectorID) || data_len >(dm->sectorSize - read_bytes))
			{
				read_bytes += data_len;
				continue; // skip from bad data 
			}
			// we should skip size ( 2B ) 
			string name = string(reinterpret_cast<char*>(data + read_bytes + sizeof(uint16)), data_len - sizeof(SectorID));
			// skip name size 
			SectorID id = *reinterpret_cast<SectorID*>(data + read_bytes + data_len - sizeof(SectorID));
			// read sector id ( 4B ) 
			read_bytes += data_len; // update read bytes
			entries.push_back({ name, id });
		}
		return entries;
	}

	void setEntries(vector<INDEX> table)
	{
		uint32 offset = sizeof(SectorID) + sizeof(uint16);

		size_t write_byte = offset;
		for (auto index : table)
		{
			// write data size 
			uint16 size = index.name.size() + sizeof(SectorID) + sizeof(uint16);
			memcpy(page->data + write_byte, &size, sizeof(uint16));
			write_byte += sizeof(uint16);
			//write name 
			uint32 name_size = index.name.size();
			memcpy(page->data + write_byte, index.name.c_str(), name_size);
			write_byte += name_size;
			// write sector id
			memcpy(page->data + write_byte, &(index.id), sizeof(SectorID));
			write_byte += sizeof(SectorID);
		}

		freeSize(page->data) = dm->sectorSize - write_byte;
		changed = true;
	}

	bool appendEntry(const INDEX& index)
	{
		uint32 name_size = index.name.size();
		uint32 total_size = name_size + sizeof(SectorID) + sizeof(uint16); // name + inodeID 
		if (free_size < total_size)
			return false;
		// write size 
		size_t write_byte = dm->sectorSize - free_size;
		*reinterpret_cast<uint16*>(page->data + write_byte) = static_cast<uint16>(total_size);
		write_byte += sizeof(uint16);
		// write name 
		memcpy(page->data + write_byte, index.name.c_str(), name_size);
		write_byte += name_size;
		// write inode id 
		*reinterpret_cast<SectorID*>(page->data + write_byte) = index.id;
		write_byte += sizeof(SectorID);
		free_size -= total_size;
		freeSize(page->data) = free_size;
		changed = true;
		return true; 
	}

	bool isEmpty() const
	{
		return (free_size == dm->sectorSize);
	}
	
	void rebind(SectorID new_id , SectorID previous = NULL_SECTOR) {
		if (changed) page->makeDirty();
		page->unpin(); // Release old
		
		page = dm->cache->GetPage(new_id); // Get new
		next = nextNode(page->data);
		free_size = freeSize(page->data);
		prev = previous; 
		changed = false;
	}

	void setPrevNode(SectorID sector_id)
	{
		prev = sector_id;
	}
	SectorID getPrevNode()
	{
		return prev;
	}
	void setNextNode(SectorID sector_id)
	{
		next = sector_id;
		changed = true;
	}
	uint16 & freeSpace()
	{
		return freeSize(page->data);
	}	
	SectorID& nextNode()
	{
		return nextNode(page->data);
	}
	void makeDirty()
	{
		changed = true;
	}	
	~TABLE_NODE()
	{
		if (changed)
		{
			page->makeDirty();
		}
		page->unpin();
	}
};

class DirectoryManager::Bucket
{
private:
	SectorID head_id;
	SectorID current_id;
	DirectoryManager* dm;
	DirectoryManager::TABLE_NODE table_node;
	vector<INDEX> entries;
	uint32 bucketIndex; 
	bool entriesChanged = false;

public:
	Bucket(uint32 bucketIndex, DirectoryManager* dm) :
		dm(dm), table_node(dm->table_index_id[bucketIndex], dm) , bucketIndex(bucketIndex)
	{
		head_id = dm->table_index_id[bucketIndex];
		current_id = head_id;
		this->getEntries(); // read first entries
	};

	INDEX findEntry(string name)
	{
		this->getEntries();
		do
		{
			auto it = std::find(entries.begin(), entries.end(), name);
			if (it != entries.end())
				return *it;
			// else move to next node
			if (table_node.nextNode() == NULL_SECTOR)
				break;
			this->operator++(); // move to next node

		} while (current_id != NULL_SECTOR);
		return {""  , NULL_SECTOR};
	}

	bool removeIndex(string name)
	{
		auto it = std::find_if(entries.begin(), entries.end(),
			[&name](const INDEX& index) {return name == index.name;});
		if (it == entries.end()) // not found
			return false;
		entries.erase(it);
		entriesChanged = true;
		// if node become free remove it ! 
		if (removeCurrentNode())
			table_node.getEntries(); 
		return true; 
	}

	bool addIndex(INDEX name__id)
	{
		// check if we have enough space write if not skip
		if (name__id.name.size() + sizeof(SectorID) + sizeof(uint16) > table_node.freeSpace())
			return false;
		table_node.appendEntry(name__id);
		return true;
	}

	void mark_as_dirty()
	{
		table_node.makeDirty();
	}

	void addNode()
	{
		// add empty node at the end
		// allocate new sector 
		SectorID new_node_id = dm->mapManager->alloc(dm->inode_ID);
		auto page = dm->cache->GetPage(new_node_id);
		memset(page->data, 0, dm->sectorSize); 
		// append empty node to end 
		while (table_node.nextNode() != NULL_SECTOR)
		{
			this->operator++(); // move to next node
		}
		// update pointers 
		table_node.setNextNode(new_node_id);
		table_node.makeDirty();
		// initialize new node
		SectorID prev_id = current_id;
		this->operator++(); // move to new node
		table_node.setNextNode(NULL_SECTOR);
		table_node.setPrevNode(prev_id);
		// set empty node data
		table_node.freeSpace() = dm->sectorSize - TABLE_NODE::headerSize();
	}
	
	bool removeCurrentNode()
	{
		// if we it was free , remove 
		if (table_node.freeSpace() < dm->sectorSize - TABLE_NODE::headerSize())
			return false;
		SectorID removedID = current_id, prev = table_node.getPrevNode();
		
		// only node
		if(prev == NULL_SECTOR && table_node.nextNode() == NULL_SECTOR)  
		{
			// do nothing , we always keep at least one node ? 
		}
		// we are in head 
		else if (prev == NULL_SECTOR) 
		{

			head_id = table_node.nextNode(); 
			dm->table_index_id[bucketIndex] = head_id; 
			current_id = head_id; // update current to head 
			// remove from pointer map 
			table_node.rebind(head_id , NULL_SECTOR);
			
			dm->mapManager->free(removedID);
		}
		// last sector
		else if (table_node.nextNode() == NULL_SECTOR) 
		{
			// use rotation system to avoid crashing ! 
			current_id = head_id; 
			// mark prev as end 
			TABLE_NODE temp(prev, dm); 
			temp.nextNode() = NULL_SECTOR; 
			temp.makeDirty(); 
			table_node.rebind(head_id , NULL_SECTOR); // jump to head  
			// remove from pointer map 
			dm->mapManager->free(removedID);
		}
		else 
		{
			// remove from linked list 
			TABLE_NODE temp(prev, dm);
			// go to next node 
			this->operator++();

			table_node.setPrevNode(prev); // overwrite on prev for new table 
			temp.nextNode() = current_id;
			temp.makeDirty();
			table_node.makeDirty();

			// remove from pointer map 
			dm->mapManager->free(removedID);
		}
		entriesChanged = false; // we can not change any things ! 
		return true; 
	}

	Bucket& operator++()
	{
		if (this->table_node.nextNode() != NULL_SECTOR)
		{
			// calculate node ID 
			SectorID prev = current_id;
			this->current_id = this->table_node.nextNode();
			// update table node
			this->update();
			this->table_node.rebind(current_id , prev); // set new current id 
			this->getEntries(); // update entries
			return *this;
		}
		throw std::out_of_range("No more bucket nodes available.");
	}

	void update()
	{
		if (entriesChanged)
		{
			table_node.setEntries(entries);
			table_node.makeDirty();
			entriesChanged = false;
		}
	}
	
	vector<INDEX> getEntries()
	{
		entries.clear();
		entries = table_node.getEntries();
		return entries;
	}

	void clearBucket()
	{
		SectorID next; 
		do {
			TABLE_NODE temp(head_id, dm);
			next = temp.nextNode();
			dm->mapManager->free(head_id);
			head_id = next;
		} while (next != NULL_SECTOR);
	}

	// get free space in current node
	size_t getFreeSpace()
	{
		return table_node.freeSpace();
	}

	vector<INDEX>::iterator  end()
	{
		return entries.end(); 
	}
	bool hasMoreNode()
	{
		return not(table_node.nextNode() == NULL_SECTOR);
	}
	~Bucket()
	{
		update();
		// if we create a empty node and in end of operation we do not fill it , we lose data ! 
		removeCurrentNode(); 
	}

	static Bucket createNewBucket(uint32 bucketIndex, DirectoryManager* dm)
	{
		if (dm->table_index_id[bucketIndex] != NULL_SECTOR)
			return Bucket(bucketIndex, dm); // already exist
		// allocate new sector 
		SectorID new_bucket_id = dm->mapManager->alloc(dm->inode_ID);
		auto page = dm->cache->GetPage(new_bucket_id);
		// initialize new node
		memset(page->data, 0, dm->sectorSize);
		page->makeDirty();
		dm->table_index_id[bucketIndex] = new_bucket_id;
		TABLE_NODE temp(new_bucket_id, dm);
		temp.nextNode() = NULL_SECTOR;
		temp.freeSpace() = dm->sectorSize - TABLE_NODE::headerSize();
		return Bucket(bucketIndex, dm);
	}
};

uint64 DirectoryManager::hash(string name)
{
	std::hash<string> hash_fn;
	return hash_fn(name);
}

void DirectoryManager::readMasterTable()
{
	if(inode_ID == NULL_INODE)
		throw std::invalid_argument("Invalid inode ID for directory.");

	const int idsPerSector = sectorSize / sizeof(SectorID);
	const int totalSectors = DEFAULT_MASTER_TABLE_SIZE / idsPerSector; 
	// all ids / total ids in one sector = number of sector
	table_index_id.clear();
	table_index_id.resize(totalSectors * idsPerSector);

	if (inodeMgr->getSize() == 0) // directory is empty
	{
		for (int i = 0; i < totalSectors; i++)
		{
			SectorID id = mapManager->alloc(inode_ID);
			auto page = cache->GetPage(id);
			memset(page->data, 0, sectorSize); // set all to null 
			page->makeDirty();
			inodeMgr->appendSector(id); // apend sector 
		}
	}

	int tableCounter = 0; 
	for (int i = 0; i < totalSectors; i++)
	{
		SectorID sector = inodeMgr->getSector(i);
		auto page = cache->GetPage(sector);
		int counter = 0; 

		SectorID* sectorData = reinterpret_cast<SectorID*>(page->data);
		for (int j = 0; j < idsPerSector; j++)
			table_index_id[tableCounter++] = sectorData[j];

		page->unpin();
	}
}

void DirectoryManager::writeMasterTable()
{
	const int idsPerSector = sectorSize / sizeof(SectorID);
	
	const int totalSectors = (DEFAULT_MASTER_TABLE_SIZE + sectorSize - 1) / sectorSize;

	for (int i = 0; i < totalSectors; i++)
	{
		SectorID sector = inodeMgr->getSector(i);
		auto page = cache->GetPage(sector);
		SectorID* sectorData = reinterpret_cast<SectorID*>(page->data);
		for (int j = 0; j < idsPerSector; j++)
			sectorData[j] = table_index_id[i * idsPerSector + j];
		page->makeDirty();
		page->unpin();
	}

}

INDEX DirectoryManager::find(string name)
{
	uint32 index = hash(name) % DEFAULT_MASTER_TABLE_SIZE;
	if (table_index_id[index] != NULL_SECTOR)
	{
		Bucket bucket(index, this);
		while(true)
		{
			auto index = bucket.findEntry(name);
			if (index.id != NULL_SECTOR)
			{
				return index; 
			}
			if (not bucket.hasMoreNode())
				break; 
			++bucket; 
		}
	}
	return { "" , NULL_SECTOR}; // bucket not exist
}

void DirectoryManager::writeBucket(INDEX index)
{
	uint32 bucketIndex = hash(index.name) % DEFAULT_MASTER_TABLE_SIZE;

	Bucket bucket = Bucket::createNewBucket(bucketIndex, this);
	while (true)
	{
		if (bucket.addIndex(index))
		{
			bucket.mark_as_dirty();
			return;
		}
		if (not bucket.hasMoreNode())
			break;
		++bucket;
	}
	// if we reach here , we need to add new node 
	bucket.addNode();
	if (!bucket.addIndex(index))
		throw std::runtime_error("Failed to add entry to directory bucket after creating new node.");
	bucket.mark_as_dirty();
}

bool DirectoryManager::removeFromBucket(string name)
{
	uint32 bucketIndex = hash(name) % DEFAULT_MASTER_TABLE_SIZE;
	if (table_index_id[bucketIndex] == NULL_SECTOR)
		return; // bucket not exist
	Bucket bucket(bucketIndex, this);
	while (true)
	{
		if (bucket.removeIndex(name)) // if can you find name here delete it 
		{
			bucket.mark_as_dirty();
			return true;
		}
		if (not bucket.hasMoreNode())
			break;
		++bucket;
	}
	return false; 
}

void DirectoryManager::clearBucket(uint32 bucketIndex)
{
	Bucket bucket(bucketIndex, this);
	bucket.clearBucket();
	table_index_id[bucketIndex] = NULL_SECTOR; // mark as free
}

DirectoryManager::DirectoryManager(inodeID inode_id, FileSystem* fs)
{
	this->inode_ID = inode_id;
	this->inodeMgr = new InodeManager(inode_id, fs);
	this->mapManager = fs->getPointerMapManager();
	this->cache = fs->getBufferCache();
	this->sectorSize = cache->getSectorSize();
	readMasterTable();
}

DirectoryManager::~DirectoryManager()
{
	writeMasterTable();
	if (inodeMgr != nullptr)
		delete inodeMgr;
}

bool DirectoryManager::exist(string name)
{
	INDEX index = find(name);
	if (index.id != NULL_SECTOR)
		return true;
	return false;
}

inodeID DirectoryManager::findInode(string name)
{
	return find(name).id;
}

void DirectoryManager::add(string name, inodeID inode_id)
{
	writeBucket({ name, inode_id });
}

void DirectoryManager::remove(string name)
{
	removeFromBucket(name);
}

vector<INDEX> DirectoryManager::bucketEntries(uint32 bucketIndex)
{
	if (table_index_id[bucketIndex] == NULL_SECTOR)
		return vector<INDEX>(); // empty bucket

	vector<INDEX> result;
	auto bucket = Bucket(bucketIndex, this);
	while (true)
	{
		auto bev = bucket.getEntries(); 
		result.insert(result.end(), bev.begin() , bev.end()); 
		if (not bucket.hasMoreNode())
			break;
		++bucket;
	}
	return result; 
}
