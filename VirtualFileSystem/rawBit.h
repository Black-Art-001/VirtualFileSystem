#pragma once
#include "types.h"
#include <string>

namespace rawBit {
	void writeStr(byte* disk, std::string data, size_t offset);
	std::string getStr(const byte* disk, size_t offset, size_t len);
	void writeInt(byte* disk, uint64 value, size_t offset, size_t size);
	uint64 getInt(const byte* disk, size_t offset, size_t size);
	void hexDump(byte* disk, size_t offset, size_t len);
}
