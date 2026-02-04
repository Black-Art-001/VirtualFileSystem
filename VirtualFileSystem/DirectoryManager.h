#pragma once


#include "types.h" 
#include "BufferCache.h" 
#include "PointerMapManager.h"
#include "InodeManager.h" 
#include "errors.h"
#include "FileSystem.h"

#include <vector>
#include <string>
#include <functional>
#include <exception>
#include <memory>

#define DEFAULT_MASTER_TABLE_SIZE 1024
#define DEFAULT_DATA_SECTION_SIZE 32

using std::string;
using std::vector;

struct INDEX {
	string name; 
	inodeID id; 
};


class DirectoryManager
{
private:
	
	inodeID inode_ID;
	InodeManager* inodeMgr = nullptr;
	PointerMapManager* mapManager;
	BufferCache* cache;
	uint32 sectorSize;

	class TABLE_NODE;
	class Bucket;

	vector <SectorID> table_index_id; // store all inode id of buckets
	uint64 hash(string name); 

	void readMasterTable();
	void writeMasterTable();

	// find in bucket 
	INDEX find(string name);
	// write in bucket 
	void writeBucket(INDEX index);
	// remove from bucket
	bool removeFromBucket(string name);

	void clearBucket(uint32 bucketIndex);

	friend class Bucket;
	friend class TABLE_NODE;
	 
public:
	DirectoryManager(inodeID inode_id, FileSystem * fs);
	~DirectoryManager();
	bool exist(string name);
	inodeID findInode(string name);
	void add(string name, inodeID inode_id);
	void remove(string name);
	vector<INDEX> bucketEntries(uint32 bucketIndex);

};
