#ifndef __CAKE_OPTION_H__
#define __CAKE_OPTION_H__

#include "encoding/utf16.h"

typedef struct Option {
    char loaded;
    String_UTF16 key;
    String_UTF16 value;
} Option;

void init_option(Option *opt);

char load_option(wchar_t *key, String_UTF16 *from, Option *dest);
void unload_option(Option *opt);

#endif