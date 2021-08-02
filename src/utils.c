#include "../include/utils.h"
#include "../include/global.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

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
