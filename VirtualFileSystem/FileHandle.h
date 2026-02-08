#pragma once

#include "types.h"
#include "FileDescriptor.h"
#include <string>

class FileSystem;

class FileHandle {
public:
    FileHandle(std::string filePath, inodeFlags mode, bool rootAccess);
    ~FileHandle();

	// prevent copying and assignment to ensure unique ownership of the file descriptor
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

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