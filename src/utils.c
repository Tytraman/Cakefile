#include "../include/utils.h"
#include "../include/global.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <windows.h>

void rem_allocate(void **array, void *start, size_t elements, size_t byteSize, unsigned long *arraySize) {
    void *end = start + (elements * byteSize);
    size_t size = (((*arraySize) * byteSize) + byteSize) - (end - *array);
    memcpy(start, end, size);
    (*arraySize) -= elements;
    *array = realloc(*array, (*arraySize) * byteSize);
}

void move_allocate(void **array, void *pToAdd, void *src, size_t elements, size_t byteSize, unsigned long *arraySize) {
    void *pTrueEnd = (*array) + (*arraySize) * byteSize;
    void *pEnd = pToAdd + elements * byteSize;
    unsigned long index = pToAdd - (*array);
    unsigned long size = pTrueEnd - pToAdd;
    (*arraySize) += elements;
    *array = realloc(*array, (*arraySize) * byteSize);
    pToAdd = (*array) + index;
    pEnd = pToAdd + elements * byteSize;
    memcpy(pEnd, pToAdd, size);
    memcpy(pToAdd, src, elements * byteSize);
}

void add_allocate(void **array, void *src, size_t elements, size_t byteSize, unsigned long *arraySize) {
    unsigned long tempArraySize = *arraySize;
    (*arraySize) += elements;
    *array = realloc(*array, (*arraySize) * byteSize);
    unsigned char *p = (unsigned char *) *array;
    memcpy(&p[tempArraySize * byteSize], src, elements * byteSize);
}

long get_file_size(FILE *file) {
    long size = 0L;
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    return size;
}

unsigned char *get_key_value(const char *key, unsigned char *fileBuffer, long fileSize, long *valueSize) {
    *valueSize = 0L;
    unsigned char *found = NULL;
    long i, j, keyLength = strlen(key);
    long newSize = fileSize - keyLength + 1;
    for(i = 0L; i < newSize; i++) {
        if(key[0] == fileBuffer[i]) {
            found = &fileBuffer[i];
            for(j = 1L; j < keyLength; j++) {
                i++;
                found++;
                if(key[j] != *found) {
                    found = NULL;
                    break;
                }
            }
            if(found) {
                found++;
                i++;
                while(i < fileSize && *found != ':') {
                    i++;
                    found++;
                }
                if(i == fileSize) return found;
                found++;
                i++;
                while(i < fileSize && (*found == ' ' || *found == '\t')) {
                    found++;
                    i++;
                }
                if(i == fileSize) return found;
                while(i < fileSize && fileBuffer[i] != '\r' && fileBuffer[i] != '\n') {
                    i++;
                    (*valueSize)++;
                }
                return found;
            }
        }
    }
    return found;
}

void copy_value(unsigned char **buffer, unsigned char *src, long valueSize) {
        *buffer = malloc(valueSize + 1);
        memcpy(*buffer, src, valueSize);
        (*buffer)[valueSize] = '\0';
}

void empty_str(unsigned char **str) {
    free(*str);
    *str = malloc(1);
    (*str)[0] = '\0';
}

char execute_command(char *command, Array_Char *out, Array_Char *err) {
    out->array  = NULL;
    out->length = 0UL;
    err->array  = NULL;
    err->length = 0UL;

    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;

    PROCESS_INFORMATION pi = { 0 };
    if(!CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) return 1;
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    DWORD stdoutReadSize = GetFileSize(stdoutRead, NULL);
    DWORD stderrReadSize = GetFileSize(stderrRead, NULL);

    DWORD read, totalRead = 0UL;
    char tempBuff[BUFF_SIZE];

    // Lecture et copie de stdout
    if(stdoutReadSize > 0UL) {
        while(1) {
            if(!ReadFile(stdoutRead, tempBuff, BUFF_SIZE, &read, NULL) || read == 0) return 2;
            totalRead += read;
            add_allocate((void **) &out->array, tempBuff, read, 1, &out->length);
            if(totalRead == stdoutReadSize) break;
        }
        out->array = realloc(out->array, out->length + 1);
        out->array[out->length] = '\0';
    }

    totalRead = 0UL;

    // Lecture et copie de stderr
    if(stderrReadSize > 0UL) {
        while(1) {
            if(!ReadFile(stderrRead, tempBuff, BUFF_SIZE, &read, NULL) || read == 0) return 2;
            totalRead += read;
            add_allocate((void **) &err->array, tempBuff, read, 1, &err->length);
            if(totalRead == stderrReadSize) break;
        }
        err->array = realloc(err->array, err->length + 1);
        err->array[err->length] = '\0';
    }

    return 0;
}

unsigned long list_files(Array_Char ***dest, Array_Char *files) {
    unsigned long size = 0UL;
    *dest = NULL;
    unsigned long i, length = 0UL;
    char *begin = files->array;
    for(i = 0UL; i < files->length; i++) {
        if(files->array[i] == '\r') {
            *dest = realloc(*dest, (size + 1) * sizeof(Array_Char *));
            (*dest)[size] = malloc(sizeof(Array_Char));
            Array_Char ar;
            ar.length = length;
            ar.array = malloc(length + 1);
            memcpy(ar.array, begin, length);
            ar.array[length] = '\0';
            (*(*dest)[size]) = ar;
            size++;
            i++;
            begin = &files->array[i + 1];
            length = 0UL;
        }else
            length++;
    }
    return size;
}

void free_list(Array_Char ***list, unsigned long size) {
    unsigned long i;
    for(i = 0UL; i < size; i++) {
        free((*(list)[i])->array);
        free((*list)[i]);
    }
    free(*list);
    list = NULL;
}

unsigned long list_o_files(Array_Char ***dest, Array_Char *cFiles, unsigned char *srcDir, long srcDirSize, unsigned char *objDir, long objDirSize) {
    Array_Char oFiles;
    oFiles.length = cFiles->length;
    oFiles.array = malloc(cFiles->length + 1);
    memcpy(oFiles.array, cFiles->array, cFiles->length + 1);
    unsigned long i;
    for(i = 0UL; i < oFiles.length; i++) {
        if(oFiles.array[i] == '.' && (oFiles.array[i + 1] == 'c' || oFiles.array[i + 1] == 'C'))
            oFiles.array[i + 1] = 'o';
    }
    unsigned long size = list_files(dest, &oFiles);
    free(oFiles.array);
    void *ptr;
    unsigned long ptrIndex;
    unsigned long length = programFilenameLength;

    unsigned char *objSlash = malloc(objDirSize + 1);
    memcpy(objSlash, objDir, objDirSize);
    objSlash[objDirSize] = '\\';

    for(i = 0UL; i < size; i++) {
        if((ptr = search_data((*dest)[i]->array, srcDir, 0UL, (*dest)[i]->length, srcDirSize))) {
            ptrIndex = ptr - (void *) (*dest)[i]->array;
            rem_allocate((void **) &(*dest)[i]->array, ptr, srcDirSize, 1, &(*dest)[i]->length);
            ptr = (*dest)[i]->array + ptrIndex;
            move_allocate((void **) &(*dest)[i]->array, ptr, objDir, objDirSize, 1, &(*dest)[i]->length);
            (*dest)[i]->array = realloc((*dest)[i]->array, (*dest)[i]->length + 1);
            (*dest)[i]->array[(*dest)[i]->length] = '\0';
        }else {
            move_allocate((void **) &(*dest)[i]->array, &(*dest)[i]->array[get_last_backslash(&programFilename[programFilenameLength - 1], programFilenameLength)], objSlash, objDirSize + 1, 1, &(*dest)[i]->length);
            (*dest)[i]->array = realloc((*dest)[i]->array, (*dest)[i]->length + 1);
            (*dest)[i]->array[(*dest)[i]->length] = '\0';
        }
    }
    free(objSlash);
    return size;
}

int mkdirs(char *path) {
    char *p = NULL;
    size_t len = strlen(path);
    char slash;

    // On enlève le dernier slash si y en a un
    if(path[len - 1] == '/' || path[len - 1] == '\\')
        path[len - 1] = '\0';

    DIR *dir = opendir(path);
    if(dir) {
        closedir(dir);
        return 1;
    }

    if(path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
        p = path + 3;
    else
        p = path + 1;

    for(; *p; p++) {
        if(*p == '/' || *p == '\\') {
            slash = *p;
            *p = '\0';
            if(
                #ifdef _WIN32
                mkdir(path)
                #else
                mkdir(path, 777)
                #endif
                != 0
            ) if(errno == EACCES) return 2;
            *p = slash;
        }
    }
    if(
        #ifdef _WIN32
        mkdir(path)
        #else
        mkdir(path, 777)
        #endif
        != 0
    ) if(errno == EACCES) return 2;
    return 0;
}

void clean(Array_Char **objFiles, unsigned long objFilesSize, char *objDir, char *exe) {
    unsigned long i;
    for(i = 0UL; i < objFilesSize; i++)
        remove(objFiles[i]->array);
    rmdir(objDir);
    remove(exe);
}

void *search_data(void *src, void *researching, unsigned long fromIndex, unsigned long srcSize, unsigned long researchSize) {
    if(fromIndex >= srcSize || srcSize < researchSize) return NULL;
    void *found = NULL;
    unsigned char *pSrc = (unsigned char *) src;
    unsigned char *pResearching = (unsigned char *) researching;
    unsigned long j, k;
    char isFound = 0;
    for(; fromIndex < srcSize - researchSize + 1; fromIndex++) {
        if(pSrc[fromIndex] == pResearching[0]) {
            k = fromIndex + 1;
            isFound = 1;
            for(j = 1; j < researchSize; j++) {
                if(pSrc[k] != pResearching[j]) {
                    isFound = 0;
                    break;
                }
                k++;
            }
        }
        if(isFound) {
            found = &pSrc[fromIndex];
            break;
        }
    }
    return found;
}

DWORD get_program_file_name(char **buffer) {
    char temp[0b1111111111111111];
    DWORD length = GetModuleFileNameA(NULL, temp, 0b1111111111111111);
    *buffer = malloc(length + 1);
    memcpy(*buffer, temp, length + 1);
    return length;
}

unsigned long get_last_backslash(char *filenameEnd, unsigned long filenameLength) {
    while(filenameLength >= 0) {
        if(*filenameEnd == '\\') return filenameLength;
        filenameEnd--;
        filenameLength--;
    }
}
