#ifndef __UTF8_H__
#define __UTF8_H__

#include "../array/base_array.h"
#include "../../include/encoding/utf16.h"

typedef struct String_UTF8 {
    Array_Data data;
    unsigned long length;
    unsigned char *bytes;
} String_UTF8;

/* ===== Initialisation ===== */

void create_string_utf8(String_UTF8 *utf);


/* ===== Ajout ===== */

char strutf8_add_wchar(String_UTF8 *dest, wchar_t value);


/* ===== Conversion ===== */

void string_utf8_to_utf16(String_UTF8 *src, String_UTF16 *dest);
void array_char_to_string_utf8(unsigned char *src, String_UTF8 *dest, unsigned long srcSize);
int string_utf8_decode(const unsigned char *src, char bytes);
char strutf8_wchar_to_byte(wchar_t value, unsigned char **buffer);
void strutf16_to_strutf8(String_UTF16 *src, String_UTF8 *dest);
void wchar_array_to_strutf8(const wchar_t *src, String_UTF8 *dest);


/* ===== Cleaner ===== */

void clean_string_utf8(String_UTF8 *utf);


/* ===== Recherches ===== */

void string_utf8_index_by_index(unsigned char *pArrayStart, unsigned char *pArrayEnd, unsigned long utfIndex, unsigned char **pStart, unsigned char **pEnd, int *bytes);



/* ===== Autres ===== */

unsigned long string_utf8_length(String_UTF8 *utf);

#endif