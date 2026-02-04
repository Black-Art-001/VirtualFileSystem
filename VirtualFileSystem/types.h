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

// dentry
struct Dentry {
    std::string name;
    inodeID inode;
    Dentry* parent;
    size_t pin_count;

    Dentry(std::string _n, inodeID _i, Dentry* _p)
        : name(_n), inode(_i), parent(_p), pin_count(0) {
    }

    void pin() { pin_count++; }
    void unPin() { if (pin_count > 0) pin_count--; }
};

//data types
typedef uint32 SectorID; 
typedef uint32 inodeID;
typedef uint64 Time;
#define NULL_SECTOR 0 

typedef unsigned char byte;

#define NULL_INODE 0
#define SYSTEM 1
#define PAGE_INODE 2

struct Cursor {
	SectorID sector; // sector by sector 
	uint32 offset;   // byte by byte 
};


