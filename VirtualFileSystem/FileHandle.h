#pragma once

#include "types.h"
#include <vector>
#include <string>

class FileSystem;
class FileDescriptor;

class FileHandle {
public:
    FileHandle(FileSystem* fs, int fd);


    ~FileHandle();

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    FileHandle(FileHandle&& other) noexcept;
    FileHandle& operator=(FileHandle&& other) noexcept;

    // --- Core API ---
    size_t read(byte* buffer, size_t len);
    size_t write(const byte* buffer, size_t len);
    void seek(int64 offset, position mode = position::Current);
    size_t tell() const;
    size_t size() const;
    void truncate();

    void close();

    bool isValid() const { return fd != -1 && fs != nullptr; }
    int getFd() const { return fd; }

private:
    FileSystem* fs;
    int fd;

    FileDescriptor* getDescriptor() const;
};