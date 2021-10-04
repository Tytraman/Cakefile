#ifndef __UTF8_H__
#define __UTF8_H__

#include "../array/base_array.h"
#include "../../include/encoding/utf16.h"

typedef struct String_UTF8 {
    Array_Data data;
    unsigned long length;
    unsigned char *bytes;
} String_UTF8;

void create_string_utf8(String_UTF8 *utf);
unsigned long string_utf8_length(String_UTF8 *utf);
void string_utf8_to_utf16(String_UTF8 *src, String_UTF16 *dest);
void clean_string_utf8(String_UTF8 *utf);
int string_utf8_decode(const unsigned char *src, char bytes);
void string_utf8_index_by_index(unsigned char *pArrayStart, unsigned char *pArrayEnd, unsigned long utfIndex, unsigned char **pStart, unsigned char **pEnd, int *bytes);
void array_char_to_string_utf8(unsigned char *src, String_UTF8 *dest, unsigned long srcSize);

#endif