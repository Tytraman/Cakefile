#include "../../include/memory/memory.h"

#include <string.h>
#include <stdlib.h>

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

void copy_value(char **buffer, unsigned char *src, long valueSize) {
        *buffer = malloc(valueSize + 1);
        memcpy(*buffer, src, valueSize);
        (*buffer)[valueSize] = '\0';
}
