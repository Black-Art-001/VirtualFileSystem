#pragma once
#include "types.h"

typedef unsigned char byte;

namespace rawBit {
	uint64 toInt(const byte *b, size_t len); 

	// size : how many bytes to write
	// len_byte : total length of byte array
	void toByte(byte *b, uint64 value, size_t size, size_t len_byte);
	
	void printBytes(byte *b, size_t size);
}
