#ifndef __WINAPI_H__
#define __WINAPI_H__

#include "../array/base_array.h"
#include "../encoding/utf16.h"

#include <windows.h>
#include <stddef.h>

#define REDIRECT_STDOUT 0x0001
#define REDIRECT_STDERR 0x0002
#define PRINT_STD       0x0004

DWORD get_program_file_name(String_UTF16 *dest);
DWORD get_current_process_location(String_UTF16 *dest);
char execute_command_ascii(char *command, Array_Char *out, Array_Char *err, char *error);
char execute_command(wchar_t *command, String_UTF16 *out, String_UTF16 *err, char *error, unsigned char flags);
unsigned long long filetime_to_ularge(FILETIME *ft);
unsigned long long get_current_time_millis();

#endif