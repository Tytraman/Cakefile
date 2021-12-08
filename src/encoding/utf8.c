#include "../../include/encoding/utf8.h"
#include "../../include/memory/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void create_string_utf8(String_UTF8 *utf) {
    utf->data.length = 0;
    utf->data.byteSize = sizeof(unsigned char);
    utf->length = 0;
    utf->bytes = NULL;
}

void clean_string_utf8(String_UTF8 *utf) {
    free(utf->bytes);
    create_string_utf8(utf);
}

unsigned long string_utf8_length(String_UTF8 *utf) {
    unsigned long length = 0UL;
    unsigned long i;
    for(i = 0UL; i < utf->data.length; i++) {
        if((utf->bytes[i] & 0b10000000) == 0)
            length++;
        else if((utf->bytes[i] & 0b11000000) == 192) {
            i++;
            while(i < utf->data.length && (utf->bytes[i] & 0b11000000) == 128)
                i++;
            length++;
            i--;
        }
    }
    if(utf->bytes[utf->data.length - 1] == '\0')
        length--;
    return length;
}

void string_utf8_to_utf16(String_UTF8 *src, String_UTF16 *dest) {
    unsigned char *arrayEnd = &src->bytes[src->data.length - 1];
    unsigned char *pStart = src->bytes, *startEncode, *endEncode;

    while(arrayEnd > pStart && *arrayEnd == '\0')
        arrayEnd--;

    clean_string_utf16(dest);
    int bytes;
    unsigned short value;
    while(pStart <= arrayEnd) {
        string_utf8_index_by_index(pStart, arrayEnd, 0L, &startEncode, &endEncode, &bytes);
        value = string_utf8_decode(startEncode, bytes);
        string_utf16_add_char(dest, value);
        pStart = endEncode;
    }
}

void strutf16_to_strutf8(String_UTF16 *src, String_UTF8 *dest) {
    create_string_utf8(dest);
    unsigned long i;
    for(i = 0; i < src->length; i++)
        strutf8_add_wchar(dest, src->characteres[i]);
    strutf8_add_wchar(dest, L'\0');
}

void wchar_array_to_strutf8(const wchar_t *src, String_UTF8 *dest) {
    create_string_utf8(dest);
    unsigned long length = wcslen(src);
    unsigned long i;
    for(i = 0; i < length + 1; i++)
        strutf8_add_wchar(dest, src[i]);
}

void string_utf8_index_by_index(unsigned char *pArrayStart, unsigned char *pArrayEnd, unsigned long utfIndex, unsigned char **pStart, unsigned char **pEnd, int *bytes) {
    unsigned long currentUtfIndex = 0UL;
    while(pArrayStart <= pArrayEnd) {
        if((*pArrayStart & 0b10000000) == 0) {              // Si c'est un caractère ASCII.
            if(currentUtfIndex == utfIndex) {
                *pStart = pArrayStart;
                *pEnd = pArrayStart + 1;
                if(bytes) *bytes = 1;
                return;
            }
            pArrayStart++;
            currentUtfIndex++;
        }else if((*pArrayStart & 0b11000000) == 192) {      // Si c'est le début d'un caractère encodé UTF-8.
            unsigned char *saveStart = pArrayStart;
            pArrayStart++;
            while(pArrayStart <= pArrayEnd && (*pArrayStart & 0b11000000) == 128)    // Tant que c'est un octet encodé.
                pArrayStart++;
            if(currentUtfIndex == utfIndex) {
                *pStart = saveStart;
                *pEnd = pArrayStart;
                if(bytes) *bytes = pArrayStart - saveStart;
                return;
            }
            currentUtfIndex++;
        }else {
            pArrayStart++;
        }
    }
}

int string_utf8_decode(const unsigned char *src, char bytes) {
    if(bytes < 1 || bytes > 6) return -1;
    if(bytes == 1) return (int) src[0];
    int i, j;
    unsigned int value = 0, temp;
    unsigned char mask;
    char moving = 0;
    switch(bytes) {
        case 2:
            mask = 0b00011111;
            break;
        case 3:
            mask = 0b00001111;
            break;
        case 4:
            mask = 0b00000111;
            break;
        case 5:
            mask = 0b00000011;
            break;
        case 6:
            mask = 0b00000001;
            break;
    }
    for(i = bytes - 1; i > 0; i--) {
        temp = src[i] & 0b00111111;
        temp = temp << moving;
        value = value | temp;
        moving += 6;
    }
    temp = src[0] & mask;
    temp = temp << moving;
    value = value | temp;
    return value;
}

void array_char_to_string_utf8(unsigned char *src, String_UTF8 *dest, unsigned long srcSize) {
    dest->data.length = srcSize;
    free(dest->bytes);
    dest->bytes = (unsigned char *) malloc(srcSize);
    memcpy(dest->bytes, src, srcSize);
    dest->length = string_utf8_length(dest);
}

char strutf8_add_wchar(String_UTF8 *dest, wchar_t value) {
    unsigned char *buffer = NULL;
    char bytes = strutf8_wchar_to_byte(value, &buffer);
    if(bytes != -1) {
        (dest->length)++;
        add_allocate((void **) &dest->bytes, buffer, bytes, dest->data.byteSize, &dest->data.length);
        free(buffer);
    }
    return bytes;
}

char strutf8_wchar_to_byte(wchar_t value, unsigned char **buffer) {
    char bytes = 0;
    if(value <= 126) bytes = 1;
    else if(value <= 2047) bytes = 2;
    else if(value <= 65535) bytes = 3;
    //else if(value <= 1114111) bytes = 4;
    else return -1;
    *buffer = malloc(bytes * sizeof(unsigned char));
    if(bytes == 1) {
        (*buffer)[0] = value & 0b01111111;
    }else {
        int i;
        for(i = bytes - 1; i > 0; i--) {
            (*buffer)[i] = (value & 0b00111111) | 0b10000000;
            value = value >> 6;
        }
        switch(bytes) {
            case 2:
                (*buffer)[0] = (value & 0b00011111) | 0b11000000;
                break;
            case 3:
                (*buffer)[0] = (value & 0b00001111) | 0b11100000;
                break;
            //case 4:
            //    (*buffer)[0] = (value & 0b00000111) | 0b11110000;
            //    break;
        }
    }
    return bytes;
}
