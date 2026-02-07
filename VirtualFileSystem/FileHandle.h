#pragma once

#include "types.h"
#include "FileDescriptor.h"
#include <string>

class FileSystem;

class FileHandle {
public:
    FileHandle(std::string filePath, int flag);
    ~FileHandle();

    // --- Core API ---
    size_t read(byte* buffer, size_t len);
    size_t write(const byte* buffer, size_t len);
    void seek(int64 offset, position mode = position::Current);
    size_t tell() const;
    size_t size() const;
    void truncate();

    void close();

    bool isValid() const { return fdId != -1 && fs != nullptr; }
    int getFd() const { return fdId; }

	static void init(FileSystem* _fs); // initialing the FH class with the fs pointer

private:
	static FileSystem* fs; // shared FileSystem pointer for all FileHandle instances
    int fdId;
	FileDescriptor* fd; // Pointer to the underlying FileDescriptor
};