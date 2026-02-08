#include "SystemKernel.h"
#include <fstream>
#include <cstring>

int64 SystemKernel::next_fd = 0; 

// Helper to find an empty slot or push back
template <typename T>
int addToVector(std::vector<T*>& vec, T* data) {
	for (size_t i = 0; i < vec.size(); ++i) {
		if (vec[i] == nullptr) {
			vec[i] = data;
			return static_cast<int>(i);
		}
	}
	vec.push_back(data);
	return static_cast<int>(vec.size() - 1);
}

SystemKernel::~SystemKernel() {
	for (auto d : devices) delete d;
	for (auto f : fsys) delete f;
	for (auto const& [id, fd] : fd_table) delete fd;
}

int SystemKernel::MountDevice(std::string path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) return -1;

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	// Allocate memory for the virtual device
	byte* buffer = new byte[size];
	if (!file.read(reinterpret_cast<char*>(buffer), size)) {
		delete[] buffer;
		return -1;
	}

	BlockDevice* newDev = new BlockDevice(buffer);
	return addToVector(devices, newDev);
}

bool SystemKernel::NewDevice(size_t size, Superblock& S, std::string path) {
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) return false;

	byte* mem = new byte[size];
	memset(mem, 0, size);
	memcpy(mem, &S, sizeof(Superblock));

	file.write(reinterpret_cast<char*>(mem), size);
	file.close();

	delete[] mem;
	return true;
}

bool SystemKernel::unMountDevice(int device_id)
{
	if (device_id < devices.size())
	{
		delete devices[device_id];
		devices[device_id] = nullptr;
	}
	return false;
}

int SystemKernel::openFS(int device_id) {
	if (device_id < 0 || device_id >= devices.size() || devices[device_id] == nullptr) {
		return -1;
	}

	FileSystem* newFs = new FileSystem(devices[device_id]);
	// Initialize root directory or load structures here if needed
	return addToVector(fsys, newFs);
}

int64 SystemKernel::addFD(FileDescriptor* fd) {
	int64 current_id = next_fd++;
	fd_table[current_id] = fd;
	return current_id;
}

bool SystemKernel::removeFD(int64 fd) {
	if (fd_table.find(fd) != fd_table.end()) {
		delete fd_table[fd];
		fd_table.erase(fd);
		return true;
	}
	return false;
}

FileDescriptor* SystemKernel::getFD(int64 fd)
{
	auto it = fd_table.find(fd);
	if (it != fd_table.end())
	{
		return it->second;
	}

	return nullptr;
}

bool SystemKernel::closeFS(int fs_id) {
	if (fs_id >= 0 && fs_id < fsys.size() && fsys[fs_id] != nullptr) {
		delete fsys[fs_id];
		fsys[fs_id] = nullptr;
		return true;
	}
	return false;
}

FileSystem* SystemKernel::getFS(int fs_id)
{
	if (fs_id >= 0 && fs_id < fsys.size() && fsys[fs_id] != nullptr) {
		return fsys[fs_id];
	}
	return nullptr;
}

