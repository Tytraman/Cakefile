#include "../../include/file/file_utils.h"

#include <stdio.h>
#include <dirent.h>

#include "../../include/error.h"

DWORD get_file_size(const wchar_t *filename) {
    HANDLE hFile = CreateFileW(
        filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_POSIX_SEMANTICS, NULL
    );
    if(hFile == NULL)
        return -1;
    DWORD fileSize = GetFileSize(hFile, NULL);
    CloseHandle(hFile);
    return fileSize;
}

unsigned long file_buffer(unsigned char **buffer, const wchar_t *filepath) {
    FILE *pFile = _wfopen(filepath, L"rb");
    if(!pFile) return -1;
    unsigned char buff[2048];
    size_t read;
    unsigned long index = 0;
    unsigned long i;
    long filesize = get_file_size(filepath);
    *buffer = (unsigned char *) malloc(filesize * sizeof(unsigned char));
    while((read = fread(buff, 1, 2048, pFile)) > 0) {
        for(i = 0; i < read; i++) {
            (*buffer)[index] = buff[i];
            index++;
        }
    }
    fclose(pFile);
    return index;
}

char open_utf8_file(String_UTF8 *utf, const wchar_t *filepath) {
    unsigned long tempSize = file_buffer(&utf->bytes, filepath);
    if(tempSize == (unsigned long) -1) return 0;
    utf->data.length = tempSize;
    utf->length = string_utf8_length(utf);
    return 1;
}

char folder_exists(wchar_t *path) {
    DWORD value = GetFileAttributesW(path);
    return (value != INVALID_FILE_ATTRIBUTES && (value & FILE_ATTRIBUTE_DIRECTORY));
}

char mkdirs(wchar_t *path) {
    String_UTF16 pathCopy;
    create_string_utf16(&pathCopy);
    string_utf16_set_value(&pathCopy, path);

    wchar_t *p = NULL;
    wchar_t slash;

    // On enlève le dernier slash si y en a un
    if(pathCopy.characteres[pathCopy.length - 1] == L'/' || pathCopy.characteres[pathCopy.length - 1] == L'\\')
        pathCopy.characteres[pathCopy.length - 1] = L'\0';

    if(folder_exists(pathCopy.characteres)) {
        free(pathCopy.characteres);
        return 1;
    }

    if(pathCopy.characteres[1] == L':' && (pathCopy.characteres[2] == L'\\' || pathCopy.characteres[2] == L'/'))
        p = pathCopy.characteres + 3;
    else
        p = pathCopy.characteres + 1;

    for(; *p; p++) {
        if(*p == L'/' || *p == L'\\') {
            slash = *p;
            *p = L'\0';
            CreateDirectoryW(pathCopy.characteres, NULL);
            *p = slash;
        }
    }
    if(!CreateDirectoryW(pathCopy.characteres, NULL)) error_create_folder(pathCopy.characteres);
    return 0;
}

char file_exists(wchar_t *filepath) {
    return GetFileAttributesW(filepath) != 0xffffffff;
}
