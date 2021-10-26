#include "../include/option.h"
#include "../include/error.h"

#include <stdlib.h>

void init_option(Option *opt) {
    create_string_utf16(&opt->key);
    create_string_utf16(&opt->value);
    opt->loaded = 0;
}

char load_option(const wchar_t *key, String_UTF16 *from, Option *dest) {
    string_utf16_set_value(&dest->key, (wchar_t *) key);
    if(!string_utf16_key_value(key, from, &dest->value)) {
        clean_string_utf16(&dest->key);
        return 0;
    }
    dest->loaded = 1;
    return 1;
}

void unload_option(Option *opt) {
    if(opt->loaded) {
        clean_string_utf16(&opt->key);
        clean_string_utf16(&opt->value);
        opt->loaded = 0;
    }
}
