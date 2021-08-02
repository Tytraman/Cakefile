#ifndef __CAKE_GLOBAL_H__
#define __CAKE_GLOBAL_H__

#include <windows.h>

#define BUFF_SIZE 2048

#define PROGRAM_STATUS_CAKEFILE_NOT_FOUND     1
#define PROGRAM_STATUS_KEY_NOT_FOUND          2
#define PROGRAM_STATUS_FILE_NOT_FOUND         3
#define PROGRAM_STATUS_DOUBLE_CLICKS          4
#define PROGRAM_STATUS_ERROR_CREATE_PROCESS   5
#define PROGRAM_STATUS_ERROR_CREATE_PIPE      6
#define PROGRAM_STATUS_ERROR_SET_HANDLE_INFOS 7

extern const char *VERSION;
extern const char *PROGRAM_NAME;

extern char programStatus;

extern HANDLE stdoutRead, stdoutWrite;
extern HANDLE stderrRead, stderrWrite;
extern HANDLE stdoutParent;
extern HANDLE stderrParent;

#endif