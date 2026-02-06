#include "FileHandle.h"
#include "FileSystem.h"
#include "FileDescriptor.h"
#include <stdexcept>

FileHandle::FileHandle(FileSystem* _fs, int _fd)
    : fs(_fs), fd(_fd) {
}

FileHandle::~FileHandle() {
    close();
}

FileHandle::FileHandle(FileHandle&& other) noexcept
    : fs(other.fs), fd(other.fd) {
    other.fs = nullptr;
    other.fd = -1;
}

FileHandle& FileHandle::operator=(FileHandle&& other) noexcept {
    if (this != &other) {
        close();
        fs = other.fs;
        fd = other.fd;
        other.fs = nullptr;
        other.fd = -1;
    }
    return *this;
}

FileDescriptor* FileHandle::getDescriptor() const {
    if (!isValid()) throw std::runtime_error("FS_ERROR: Attempted to use an invalid FileHandle.");

    FileDescriptor* desc = fs->get_fd_object(fd);
    if (!desc) throw std::runtime_error("FS_ERROR: File descriptor not found in system table.");

    return desc;
}

size_t FileHandle::read(byte* buffer, size_t len) {
    return getDescriptor()->read(buffer, len);
}

size_t FileHandle::write(const byte* buffer, size_t len) {
    return getDescriptor()->write(const_cast<byte*>(buffer), len);
}

void FileHandle::seek(int64 offset, position mode) {
    getDescriptor()->seek(offset, mode);
}

size_t FileHandle::tell() const {
    return getDescriptor()->tell();
}

size_t FileHandle::size() const {
    return getDescriptor()->inode->getSize();
}

void FileHandle::truncate() {
    getDescriptor()->truncate();
}

void FileHandle::close() {
    if (isValid()) {
        fs->close(fd);
        fs = nullptr;
        fd = -1;
    }
}