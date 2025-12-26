#pragma once

enum MetaDataOffset : unsigned int
{
	mode_offset = 0,
	linkCount_offset = 2,
	// info : int 32
	inodeID_offset = 4,
	parentID_offset = 8,
	// ID : int 16
	uid_offset = 12,
	gid_offset = 14,
	// size : int 64
	size_offset = 16,
	block_offset = 24,
	// time : int 64
	atime_offset = 28,
	mtime_offset = 36,
	ctime_offset = 44,
	// storage blocks
	// direct blocks : uint 32[12]
	direct_offset = 52, // extend
	indirect_offset = 148,
	doubleIndirect_offset = 152,
	tripleIndirect_offset = 156,
	// write position
	currentSector_offset = 160,
	offset_offset = 164,
	padding_offset = 168
};

enum MetaDateSize : unsigned int
{
	mode_size = 2 , 
	linkCount_size = 2,
	inodeID_size = 4,
	parentID_size = 4,
	uid_size = 2,
	gid_size = 2,
	size_size = 8,
	block_size = 4,
	atime_size = 8,
	mtime_size = 8,
	ctime_size = 8,
	direct_size = 96,
	indirect_size = 4,
	doubleIndirect_size = 4,
	tripleIndirect_size = 4,
	currentSector_size = 4,
	offset_size = 4,
	padding_size = 92
};

#define METADATA_SIZE 256 
#define DIRECT_BLOCK_COUNT 12
#define INDIRECT_BLOCK_COUNT 1
#define DOUBLE_INDIRECT_BLOCK_COUNT 1
#define TRIPLE_INDIRECT_BLOCK_COUNT 1

