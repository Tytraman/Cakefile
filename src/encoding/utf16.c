#include "../../include/encoding/utf16.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

void set_console_UTF16() {
    // On met la console en UTF16 pour pouvoir écrire avec de l'unicode
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);
}

void create_string_utf16(String_UTF16 *utf) {
    utf->length = 0;
    utf->characteres = NULL;
}

void clean_string_utf16(String_UTF16 *utf) {
    free(utf->characteres);
    create_string_utf16(utf);
}

void string_utf16_set_value(String_UTF16 *utf, wchar_t *str) {
    utf->length = wcslen(str);
    free(utf->characteres);
    utf->characteres = malloc(utf->length * sizeof(wchar_t) + sizeof(wchar_t));
    memcpy(utf->characteres, str, utf->length * sizeof(wchar_t));
    utf->characteres[utf->length] = L'\0';
}

void string_utf16_add(String_UTF16 *utf, wchar_t *str) {
    unsigned long lastLength = utf->length;
    unsigned long strLength = wcslen(str);
    utf->length += strLength;
    utf->characteres = realloc(utf->characteres, utf->length * sizeof(wchar_t) + sizeof(wchar_t));
    memcpy(&utf->characteres[lastLength], str, strLength * sizeof(wchar_t));
    utf->characteres[utf->length] = L'\0';
}

void string_utf16_add_char(String_UTF16 *utf, wchar_t c) {
    utf->length += 1;
    utf->characteres = realloc(utf->characteres, utf->length * sizeof(wchar_t) + sizeof(wchar_t));
    memcpy(&utf->characteres[utf->length - 1], &c, sizeof(wchar_t));
    utf->characteres[utf->length] = L'\0';
}

char string_utf16_key_value(const wchar_t *key, String_UTF16 *src, String_UTF16 *dest) {
    unsigned long valueLength = 0;
    size_t keyLength = wcslen(key);
    unsigned long newSize = src->length - keyLength + 1;
    unsigned long i, j;
    wchar_t *found;
    for(i = 0; i < newSize; i++) {
        if(key[0] == src->characteres[i]) {
            found = &src->characteres[i];
            for(j = 1; j < keyLength; j++) {
                i++;
                found++;
                if(key[j] != *found) {
                    found = NULL;
                    break;
                }
            }
            if(found) {
                i++;
                found++;
                while(i < src->length && *found != L':') {
                    if(*found == L'\r' || *found == L'\n') return 0;
                    i++;
                    found++;
                }
                if(i == src->length) return 0;
                i++;
                found++;
                while(i < src->length && (*found == L' ' || *found == L'\t')) {
                    i++;
                    found++;
                }
                if(*found == L'\r' || *found == L'\n') return 0;
                if(i == src->length)                   return 0;
                while(i < src->length && src->characteres[i] != L'\r' && src->characteres[i] != L'\n') {
                    i++;
                    valueLength++;
                }
                clean_string_utf16(dest);
                dest->length = valueLength;
                dest->characteres = malloc(valueLength * sizeof(wchar_t) + sizeof(wchar_t));
                memcpy(dest->characteres, found, valueLength * sizeof(wchar_t));
                dest->characteres[valueLength] = L'\0';
                return 1;
            }
        }
    }
    return 0;
}

void string_utf16_empty(String_UTF16 *utf) {
    utf->length = 0;
    free(utf->characteres);
    utf->characteres = malloc(sizeof(wchar_t));
    utf->characteres[0] = L'\0';
}

void string_utf16_add_bytes(String_UTF16 *utf, unsigned char *bytes, unsigned long size) {
    unsigned long lastLength = utf->length;
    utf->length += size;
    utf->characteres = realloc(utf->characteres, utf->length * sizeof(wchar_t));
    memcpy(&utf->characteres[lastLength], bytes, size * sizeof(wchar_t));
}

unsigned long string_utf16_replace_all_char(String_UTF16 *utf, wchar_t old, wchar_t replacement) {
    unsigned long number = 0, i;
    for(i = 0; i < utf->length; i++)
        if(utf->characteres[i] == old)
            utf->characteres[i] = replacement;
    return number;
}

char string_utf16_remove(String_UTF16 *utf, wchar_t *str) {
    wchar_t *start = string_utf16_search(utf, str);
    if(start == NULL) return 0;
    size_t length = wcslen(str);
    memcpy(start, utf->characteres + length, (utf->length - length) * sizeof(wchar_t) + sizeof(wchar_t));
    utf->length -= length;
    utf->characteres = realloc(utf->characteres, utf->length * sizeof(wchar_t) + sizeof(wchar_t));
    return 1;
}

wchar_t *string_utf16_search(String_UTF16 *utf, wchar_t *research) {
    size_t length = wcslen(research);
    wchar_t *found = NULL;

    unsigned long i, j;
    for(i = 0; i < utf->length - length; i++) {
        if(utf->characteres[i] == research[0]) {
            found = &utf->characteres[i++];
            for(j = 1; j < length; j++) {
                if(utf->characteres[i++] != research[j]) {
                    found = NULL;
                    break;
                }
            }
            if(found) break;
        }
    }

    return found;
}

wchar_t *string_utf16_search_from(String_UTF16 *utf, wchar_t *research, unsigned long *index) {
    size_t length = wcslen(research);
    wchar_t *found = NULL;

    unsigned long j;
    for(; *index < utf->length - length; (*index)++) {
        if(utf->characteres[*index] == research[0]) {
            found = &utf->characteres[(*index)++];
            for(j = 1; j < length; j++) {
                if(utf->characteres[(*index)++] != research[j]) {
                    found = NULL;
                    break;
                }
            }
            if(found) break;
        }
    }

    return found;
}

char string_utf16_replace(String_UTF16 *utf, wchar_t *old, wchar_t *replacement) {
    wchar_t *ptr = string_utf16_search(utf, old);
    if(ptr == NULL) return 0;

    size_t oldLength = wcslen(old);
    size_t replacementLength = wcslen(replacement);

    if(oldLength > replacementLength) {
        memcpy(ptr + replacementLength, ptr + oldLength, (utf->length - ((ptr - utf->characteres) + oldLength)) * sizeof(wchar_t) + sizeof(wchar_t));
        memcpy(ptr, replacement, replacementLength * sizeof(wchar_t));
        utf->length -= oldLength - replacementLength;
        utf->characteres = realloc(utf->characteres, utf->length * sizeof(wchar_t) + sizeof(wchar_t));
    }else if(oldLength < replacementLength) {
        utf->length += replacementLength - oldLength;
        utf->characteres = realloc(utf->characteres, utf->length * sizeof(wchar_t) + sizeof(wchar_t));
        memcpy(ptr + replacementLength, ptr + oldLength, (utf->length - ((ptr - utf->characteres) + oldLength)) * sizeof(wchar_t) + sizeof(wchar_t));
        memcpy(ptr, replacement, replacementLength * sizeof(wchar_t));
    }else
        memcpy(ptr, replacement, replacementLength * sizeof(wchar_t));

    return 1;
}

void string_utf16_insert(String_UTF16 *utf, wchar_t *str) {
    size_t length = wcslen(str);
    utf->characteres = realloc(utf->characteres, (utf->length + length) * sizeof(wchar_t) + sizeof(wchar_t));
    memcpy(utf->characteres + length, utf->characteres, utf->length * sizeof(wchar_t) + sizeof(wchar_t));
    memcpy(utf->characteres, str, length * sizeof(wchar_t));
}

void string_utf16_copy(String_UTF16 *from, String_UTF16 *to) {
    create_string_utf16(to);
    string_utf16_set_value(to, from->characteres);
}

char string_utf16_remove_from_index(String_UTF16 *utf, unsigned long index) {
    if(index < utf->length) {
        utf->length = index - 1;
        utf->characteres = realloc(utf->characteres, utf->length * sizeof(wchar_t) + sizeof(wchar_t));
        utf->characteres[utf->length] = L'\0';
        return 1;
    }
    return 0;
}

char string_utf16_remove_before_index(String_UTF16 *utf, unsigned long index) {
    if(index < utf->length) {
        utf->length -= index;
        memcpy(utf->characteres, &utf->characteres[index], utf->length * sizeof(wchar_t) + sizeof(wchar_t));
        utf->characteres = realloc(utf->characteres, utf->length * sizeof(wchar_t) + sizeof(wchar_t));
    }
    return 0;
}

char string_utf16_remove_part_from_end(String_UTF16 *utf, wchar_t delim) {
    wchar_t *ptr = &utf->characteres[utf->length - 1];
    while(ptr >= utf->characteres) {
        if(*ptr == delim) {
            string_utf16_remove_from_index(utf, (ptr - utf->characteres) + 1);
            return 1;
        }
        ptr--;
    }
    return 0;
}
