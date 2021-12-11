#include "../file_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <dirent.h>

#include "../../error.h"

//TODO: portage: get_file_size
char get_file_size(const wchar_t *filename, unsigned long long *size) {
    #ifdef _WIN32
    *size = 0;
    HANDLE hFile = CreateFileW(
        filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_POSIX_SEMANTICS, NULL
    );
    if(hFile == NULL) return 0;
    DWORD highOrder;
    DWORD lowOrder = GetFileSize(hFile, &highOrder);
    CloseHandle(hFile);

    if(lowOrder == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) return 0;

    *size = ((unsigned long long) highOrder << 8) | lowOrder;
    #endif

    return 1;
}

char file_buffer(unsigned char **buffer, const wchar_t *filepath, unsigned long long *size) {
    if(size) *size = 0;

    FILE *pFile = _wfopen(filepath, L"rb");
    if(!pFile) return 0;

    unsigned char buff[2048];

    unsigned long long i = 0;
    unsigned long long fileSize;
    unsigned long long read;
    if(!get_file_size(filepath, &fileSize)) {
        fclose(pFile);
        return 0;
    }
    *buffer = (unsigned char *) malloc(fileSize * sizeof(unsigned char));
    while((read = fread(buff, 1, 2048, pFile)) > 0) {
        memcpy((*buffer) + i, buff, read);
        i += read;
    }
    fclose(pFile);
    if(size) *size = fileSize;
    return 1;
}

char open_utf8_file(String_UTF8 *utf, const wchar_t *filepath) {
    unsigned long long size;
    if(!file_buffer(&utf->bytes, filepath, &size)) return 0;

    utf->data.length = size + 1;
    utf->bytes = (unsigned char *) realloc(utf->bytes, utf->data.length);
    utf->bytes[utf->data.length - 1] = '\0';
    utf->length = string_utf8_length(utf);
    return 1;
}

//TODO: portage: folder_exists
char folder_exists(wchar_t *path) {
    #ifdef _WIN32
    DWORD value = GetFileAttributesW(path);
    return (value != INVALID_FILE_ATTRIBUTES && (value & FILE_ATTRIBUTE_DIRECTORY));
    #endif
    return 1;
}

//TODO: portage: mkdirs
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
            #ifdef _WIN32
            CreateDirectoryW(pathCopy.characteres, NULL);
            #endif
            *p = slash;
        }
    }
    #ifdef _WIN32
    if(!CreateDirectoryW(pathCopy.characteres, NULL)) error_create_folder(pathCopy.characteres);
    #endif
    return 0;
}

char file_exists(wchar_t *filepath) {
    return GetFileAttributesW(filepath) != 0xffffffff;
}
