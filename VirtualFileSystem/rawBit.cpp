#include "rawBit.h"
#include <iostream>
using std::cout;
using std::endl;
using std::hex;
using std::dec;
using std::string;

void rawBit::writeStr(byte* disk, string data, size_t offset) {
    for (size_t i = 0; i < data.size(); i++)
    {
        disk[offset + i] = (byte)data[i];
    }
}

string rawBit::getStr(const byte* disk, size_t offset, size_t len) {
    string result = "";
    for (size_t i = offset; i < offset + len; i++)
    {
        result += disk[i];
    }
    return result;
}

void rawBit::writeInt(byte* disk, uint64 value, size_t offset, size_t size) {
    for (size_t i = 0; i < size; i++) {
        disk[offset + i] = (value >> (8 * i)) & 0xFF;
    }
}

uint64 rawBit::getInt(const byte* disk, size_t offset, size_t size) {
    uint64 result = 0;
    for (size_t i = 0; i < size; i++) {
        result |= ((uint64)disk[offset + i] << (8 * i));
    }
    return result;
}

void rawBit::hexDump(byte* disk, size_t offset, size_t len) {
    cout << " index  | hex   | decimal | char" << endl;
    cout << "-----------------------------" << endl;
    for (size_t i = offset; i < offset + len; i++)
    {
        cout << i << "\t| " << hex << (unsigned int)disk[i] << "\t| " << dec << (unsigned int)disk[i] << "\t  | " << disk[i] << endl;
    }
}