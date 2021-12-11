#include "../../include/memory/memory.h"

#include <string.h>
#include <stdlib.h>

void rem_allocate(void **array, void *start, size_t elements, size_t byteSize, unsigned long *arraySize) {
    char *end = (char *) start + (elements * byteSize);
    size_t size = (((*arraySize) * byteSize) + byteSize) - ((char *) end - (char *) *array);
    memcpy(start, end, size);
    (*arraySize) -= elements;
    *array = realloc(*array, (*arraySize) * byteSize);
}

void move_allocate(void **array, void *pToAdd, void *src, size_t elements, size_t byteSize, unsigned long *arraySize) {
    char *pTrueEnd = ((char *) *array) + (*arraySize) * byteSize;
    char *pEnd = (char *) pToAdd + elements * byteSize;
    unsigned long index = (char *) pToAdd - ((char *) *array);
    unsigned long size = pTrueEnd - (char *) pToAdd;
    (*arraySize) += elements;
    *array = realloc(*array, (*arraySize) * byteSize);
    pToAdd = ((char *) *array) + index;
    pEnd = (char *) pToAdd + elements * byteSize;
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

void copy_value(char **buffer, unsigned char *src, long valueSize) {
        *buffer = (char *) malloc(valueSize + 1);
        memcpy(*buffer, src, valueSize);
        (*buffer)[valueSize] = '\0';
}
