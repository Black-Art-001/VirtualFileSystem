#include "FileHandle.h"
#include "FileSystem.h"
#include "FileDescriptor.h"
#include <stdexcept>

FileHandle::FileHandle(std::string filePath, int flag) {
    if (!fs) {
        throw std::runtime_error("FileSystem not initialized. Call FileHandle::init() first.");
    }
    fdId = fs->open(filePath, flag);
    if (fdId == -1) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    fd = fs->get_fd_object(fdId);
}

FileHandle::~FileHandle() {
    close();
}

size_t FileHandle::read(byte* buffer, size_t len) {
    return fd->read(buffer, len);
}

size_t FileHandle::write(const byte* buffer, size_t len) {
    return fd->write(const_cast<byte*>(buffer), len);
}

void FileHandle::seek(int64 offset, position mode)
{
	fd->seek(offset, mode);
}

size_t FileHandle::tell() const {
    return fd->tell();
}

size_t FileHandle::size() const {
    return fd->getSize();
}

void FileHandle::truncate() {
    fd->truncate();
}

void FileHandle::close() {
    if (isValid()) {
        fs->close(fdId);
        fdId = -1;
		fd = nullptr;
    }
}

void FileHandle::init(FileSystem* _fs)
{
	fs = _fs;
}

FileSystem* FileHandle::fs = nullptr; // Initialize static member