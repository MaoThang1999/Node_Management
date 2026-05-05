/**
 * @brief Reads the first 8 bits from buffer to determine message type.
 * @param buffer Raw byte buffer.
 * @return Extracted TypeMessage.
 */

#include "Utils.h"



TypeMessage checkMessageType(char* buffer){
    TypeMessage typeMessage;
    int offset = 0;
    typeMessage = (TypeMessage)(get_bits(buffer, offset, 8));
    return typeMessage;
}

void set_bits(char* buffer, int& offset, uint32_t value, int bitCount){
    for(int i = bitCount - 1; i >= 0; --i){
        int byteIndex = offset/8;
        int bitIndex = 7 - (offset % 8);

        buffer[byteIndex] |= ((value >> i) & 0x1) << bitIndex;
        
        offset++;
    }
}

void set_bits(char* buffer, int& offset, char* str, int strLen){
    for(int i = 0; i <strLen; ++i){
        char currentChar = str[i];
        for(int bit = 7; bit >= 0; --bit){
            int byteIndex = offset/8;
            int bitIndex = 7 - (offset % 8);

            buffer[byteIndex] |= ((currentChar >> bit) & 0x1) << bitIndex;
            ++offset;

        }

        
    }
}

int get_bits(char* buffer, int& bitOffset, int numBits){
    int value = 0;
    for(int i = 0; i < numBits; i++){
        int byteIndex = bitOffset/8;
        int bitIndex = 7 - (bitOffset % 8);

        if(buffer[byteIndex] & (1 << bitIndex)){
            value |= (1 << (numBits - 1 - i));
        }
        ++bitOffset;
    }
    return value;
}

void get_bits(char* buffer, int& bitOffset, char* str, int strLen){
    memset(str, 0, strLen);
    for(int i = 0; i < strLen; ++i){
        char cValue = 0; 
        for(int bit = 7; bit >= 0; --bit){
            int byteIndex = bitOffset/8;
            int bitIndex = 7 - (bitOffset % 8);
            if(buffer[byteIndex] & (1 << bitIndex)){
                cValue |= (1 << bit);
            }
            ++bitOffset;
        }
        str[i] = cValue;
    }
}