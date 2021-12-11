#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

#include "../encoding/utf8.h"

#include <stddef.h>

char file_buffer(unsigned char **buffer, const wchar_t *filepath, unsigned long long *size);

/*
    Ouvre un fichier encodé en UTF-8 et le stock dans un `String_UTF8`.

    Retourne 1 en cas de succès, sinon 0.
*/
char open_utf8_file(String_UTF8 *utf, const wchar_t *filepath);

char get_file_size(const wchar_t *filename, unsigned long long *size);

char mkdirs(wchar_t *path);

char folder_exists(wchar_t *path);

char file_exists(wchar_t *filepath);

#endif