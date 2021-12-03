#ifndef __CAKE_GLOBAL_H__
#define __CAKE_GLOBAL_H__

#include "option.h"

#include <windows.h>

/*

    Fichier pour définir toutes les variables et les constantes
    communes à "tous" les fichiers.
    
*/

#define BUFF_SIZE 2048

#define MODE_ALL   1
#define MODE_RESET 2
#define MODE_LINK  3
#define MODE_CLEAN 4
#define MODE_EXEC  5

#define C_LANGUAGE   1
#define CPP_LANGUAGE 2

extern HANDLE g_Out;
extern CONSOLE_SCREEN_BUFFER_INFO g_ScreenInfo;

extern short g_LastX;
extern short g_LastY;
extern char g_DrawProgressBar;
extern short g_ProgressBarWidthScale;
extern wchar_t g_ProgressBarEmptyChar;
extern wchar_t g_ProgressBarFillChar;

extern char g_ModeLanguage;

extern const char    *VERSION;
extern const char    *PROGRAM_NAME;
extern const wchar_t *OPTIONS_FILENAME;

extern const char *STDERR;
extern const char *STDOUT;
extern const char *STDIN;

extern const wchar_t FILE_SEPARATOR;
extern const wchar_t REVERSE_FILE_SEPARATOR;

extern char g_Mode;
extern BOOL g_AutoExec;

extern unsigned long g_NeedCompileNumber;
extern unsigned long g_CompileNumber;
extern unsigned long g_CurrentCompile;

extern String_UTF16 g_ProgramFilename;
extern String_UTF16 g_Pwd;

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

extern String_UTF16 g_Compiler;

#endif