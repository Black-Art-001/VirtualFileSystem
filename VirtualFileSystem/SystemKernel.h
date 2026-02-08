#pragma once
#include "types.h"
#include "FileSystem.h"
#include "BlockDevice.h"
#include <string>
#include <vector>
#include <unordered_map>

class SystemKernel {
private:
    static std::vector<BlockDevice*> devices;
    static std::vector<FileSystem*> fsys;
    // Map File Descriptor ID to the actual FileDescriptor object
    static std::unordered_map<int64, FileDescriptor*> fd_table;
    static int64 next_fd; 
public:
    SystemKernel() = default;
    ~SystemKernel(); // Important for cleaning up memory

    // Device Management
    static int MountDevice(std::string path);
    static bool NewDevice(size_t size, Superblock& S, std::string path);
    static bool unMountDevice(int device_id);
    // File System Management
    static int openFS(int device_id);
    static bool closeFS(int fs_id);

    // File Descriptor Management
    static int64 addFD(FileDescriptor* fd);
    static bool removeFD(int64 fd);

    static FileDescriptor* getFD(int64 fd);
};

int64 SystemKernel::next_fd = 0; 