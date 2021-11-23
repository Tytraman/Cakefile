#ifndef __CAKE_GLOBAL_H__
#define __CAKE_GLOBAL_H__

#include "option.h"

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

#define C_LANGUAGE   1
#define CPP_LANGUAGE 2

extern char g_ModeLanguage;

extern const char    *VERSION;
extern const char    *PROGRAM_NAME;
extern const wchar_t *OPTIONS_FILENAME;

extern const char *STDERR;
extern const char *STDOUT;
extern const char *STDIN;

extern char g_Mode;
extern BOOL g_AutoExec;

extern String_UTF16 programFilename;
extern String_UTF16 pwd;

extern char programStatus;

extern Option o_Language;       // Obligatoire
extern Option o_ObjDir;         // Obligatoire
extern Option o_BinDir;         // Obligatoire
extern Option o_ExecName;       // Obligatoire
extern Option o_Includes;
extern Option o_Libs;
extern Option o_CompileOptions;
extern Option o_LinkOptions;
extern Option o_LinkLibs;
extern Option o_AutoExec;
extern Option o_ExecArgs;

extern String_UTF16 compiler;

#endif