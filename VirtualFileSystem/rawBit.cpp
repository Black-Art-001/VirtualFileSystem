#include "rawBit.h"
#include <iostream>

using std::cout; 
using std::endl;

uint64 rawBit::toInt(const byte* b, size_t len)
{
    uint64 result = 0;
    for (size_t i = 0; i < len; ++i)
    {
        result = (result << 8) | b[i];
    }
    return result;
}

void rawBit::toByte(byte* b, uint64 value, size_t size, size_t len_byte)
{
    for (size_t i = 0; i < size; i++)
    {
        // 0 0 0 0  0 0 1 1
        b[len_byte - 1 - i] = value % 256;
        value /= 256;
        if (value == 0)
            break;
    }
}


void rawBit::printBytes(byte b[], size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        cout << (uint32)b[i] << " ";
    }
    cout << endl;
}