#ifndef __UTF16_H__
#define __UTF16_H__

#include <stddef.h>

typedef struct String_UTF16 {
    unsigned long length;
    wchar_t *characteres;
} String_UTF16;


/* ===== Initialisation ===== */

void create_string_utf16(String_UTF16 *utf);
void string_utf16_copy(String_UTF16 *from, String_UTF16 *to);



/* ===== Setter ===== */

void string_utf16_set_value(String_UTF16 *utf, wchar_t *str);



/* ===== Ajout ===== */

void string_utf16_add(String_UTF16 *utf, wchar_t *str);
void string_utf16_add_char(String_UTF16 *utf, wchar_t c);
void string_utf16_add_bytes(String_UTF16 *utf, unsigned char *bytes, unsigned long size);
void string_utf16_insert(String_UTF16 *utf, wchar_t *str);



/* ===== Suppression ===== */

char string_utf16_remove(String_UTF16 *utf, wchar_t *str);
char string_utf16_remove_from_index(String_UTF16 *utf, unsigned long index);
char string_utf16_remove_before_index(String_UTF16 *utf, unsigned long index);
char string_utf16_remove_part_from_end(String_UTF16 *utf, wchar_t delim);



/* ===== Remplacement ===== */

unsigned long string_utf16_replace_all_char(String_UTF16 *utf, wchar_t old, wchar_t replacement);
char string_utf16_replace(String_UTF16 *utf, wchar_t *old, wchar_t *replacement);



/* ===== Cleaner ===== */

void clean_string_utf16(String_UTF16 *utf);
void string_utf16_empty(String_UTF16 *utf);



/* ===== Recherches ===== */

wchar_t *string_utf16_search(String_UTF16 *utf, wchar_t *research);
wchar_t *string_utf16_search_from(String_UTF16 *utf, wchar_t *research, unsigned long *index);
char string_utf16_key_value(const wchar_t *key, String_UTF16 *src, String_UTF16 *dest);



/* ===== Autres ===== */

void set_console_UTF16();

#endif