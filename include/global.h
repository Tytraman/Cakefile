#ifndef __CAKE_GLOBAL_H__
#define __CAKE_GLOBAL_H__

#include "encoding/utf16.h"

#include <windows.h>

/*

    Fichier pour définir toutes les variables et les constantes
    communes à "tous" les fichiers.
    
*/

#define BUFF_SIZE 2048

#define PROGRAM_STATUS_CAKEFILE_NOT_FOUND     1
#define PROGRAM_STATUS_KEY_NOT_FOUND          2
#define PROGRAM_STATUS_FILE_NOT_FOUND         3
#define PROGRAM_STATUS_DOUBLE_CLICKS          4
#define PROGRAM_STATUS_ERROR_CREATE_PROCESS   5
#define PROGRAM_STATUS_ERROR_CREATE_PIPE      6
#define PROGRAM_STATUS_ERROR_SET_HANDLE_INFOS 7

#define MODE_ALL   1
#define MODE_RESET 2
#define MODE_LINK  3
#define MODE_CLEAN 4

extern const char *VERSION;
extern const char *PROGRAM_NAME;

extern const char *STDERR;
extern const char *STDOUT;
extern const char *STDIN;

extern char mode;

extern String_UTF16 programFilename;
extern String_UTF16 pwd;

extern char programStatus;

// Pointeur vers stdout (WINAPI).
extern HANDLE stdoutParent;
// Pointeur vers stderr (WINAPI).
extern HANDLE stderrParent;


extern String_UTF16 srcDir;
extern String_UTF16 objDir;
extern String_UTF16 binDir;

extern String_UTF16 includes;
extern String_UTF16 libs;

extern String_UTF16 compileOptions;
extern String_UTF16 linkOptions;
extern String_UTF16 linkLibs;

extern String_UTF16 exec;

#endif