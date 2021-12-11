#ifndef __BASE_ARRAY_H__
#define __BASE_ARRAY_H__

typedef struct Array_Data {
    unsigned long length;
    unsigned char byteSize;
} Array_Data;

typedef struct Array_Char {
    unsigned long length;
    char *array;
} Array_Char;

#endif