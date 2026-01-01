#pragma once

//base types
typedef long long int int64;
typedef unsigned long long int uint64;
typedef int int32;
typedef unsigned int uint32;
typedef char int8;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef short int16;


//cache constants
#define MAX_PAGE_SIZE 4096 
#define SECTOR_SIZE 1024
#define INODE_SIZE 256

//data types
typedef uint32 SectorID; 
typedef uint32 InodID;
typedef uint64 Time;
#define NULL_SECTOR 0 

typedef unsigned char byte;

typedef uint32 inodeID; 
#define NULL_INODE 0
#define SYSTEM 1
#define PAGE_INODE 2
#define NULL_INODE 0 
#define PAGE_INODE 1

struct Cursor {
	SectorID sector; // sector by sector 
	uint32 offset;   // byte by byte 
};


