#ifndef __UTF16_H__
#define __UTF16_H__

#include <stddef.h>


typedef struct String_UTF16 {
    unsigned long length;
    wchar_t *characteres;
} String_UTF16;

typedef struct String_UTF16_Reader {
    String_UTF16 *utf;
    String_UTF16 *lastPtr;
    unsigned long index;
} String_UTF16_Reader;


/* ===== Initialisation ===== */

void create_string_utf16(String_UTF16 *utf);
void string_utf16_copy(String_UTF16 *from, String_UTF16 *to);
char string_utf16_copy_between(String_UTF16 *from, String_UTF16 *to, unsigned long begin, unsigned long end);

void init_strutf16_reader(String_UTF16_Reader *reader, String_UTF16 *utf);
String_UTF16 *strutf16_getline(String_UTF16_Reader *reader);



/* ===== Setter ===== */

void string_utf16_set_value(String_UTF16 *utf, wchar_t *str);
unsigned long string_utf16_lower(String_UTF16 *utf);
unsigned long string_utf16_upper(String_UTF16 *utf);



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
unsigned long string_utf16_rtrim(String_UTF16 *utf, wchar_t charactere);
char strutf16_remove_index(String_UTF16 *utf, unsigned long index);



/* ===== Remplacement ===== */

unsigned long string_utf16_replace_all_char(String_UTF16 *utf, wchar_t old, wchar_t replacement);
char string_utf16_replace(String_UTF16 *utf, wchar_t *old, wchar_t *replacement);
char strutf16_replace_from_end(String_UTF16 *utf, wchar_t *old, wchar_t *replacement);



/* ===== Cleaner ===== */

void clean_string_utf16(String_UTF16 *utf);
void string_utf16_empty(String_UTF16 *utf);
void free_strutf16(String_UTF16 *utf);



/* ===== Recherches ===== */

wchar_t *string_utf16_search(String_UTF16 *utf, wchar_t *research);
wchar_t *strutf16_search_from_end(String_UTF16 *utf, wchar_t *research);
wchar_t *string_utf16_find(String_UTF16 *utf, wchar_t research, unsigned long *index);
wchar_t *string_utf16_search_from(String_UTF16 *utf, wchar_t *research, unsigned long *index);
char string_utf16_key_value(const wchar_t *key, String_UTF16 *src, String_UTF16 *dest);
unsigned long string_utf16_number_of(String_UTF16 *utf, wchar_t charactere);



/* ===== Autres ===== */

void set_console_UTF16();
void strutf16_print_hexa(String_UTF16 *utf);

#endif