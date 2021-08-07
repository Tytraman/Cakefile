#ifndef __CAKE_GLOBAL_H__
#define __CAKE_GLOBAL_H__

#include "utils.h"

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

extern Array_Char programFilename;
extern Array_Char pwd;

extern char programStatus;

// Stock les sorties des commandes.
extern HANDLE stdoutRead, stdoutWrite;
// Stock les erreurs des commandes.
extern HANDLE stderrRead, stderrWrite;
// Pointeur vers stdout (WINAPI).
extern HANDLE stdoutParent;
// Pointeur vers stderr (WINAPI).
extern HANDLE stderrParent;


extern long srcDirLength;
extern unsigned char *srcDir;

extern long objDirLength;
extern unsigned char *objDir;

extern long binDirLength;
extern unsigned char *binDir;

extern long includesLength;
extern unsigned char *includes;

extern long libsLength;
extern unsigned char *libs;

extern long compileOptionsLength;
extern unsigned char *compileOptions;

extern long linkOptionsLength;
extern unsigned char *linkOptions;

extern Array_Char exec;

#endif