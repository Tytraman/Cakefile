#ifndef __CAKEFILE_GLOBAL_H__
#define __CAKEFILE_GLOBAL_H__

#include "include/libcake/def.h"
#include "include/libcake/strutf8.h"
#include "include/libcake/fileobject.h"

// Si le bit est à 1, c'est que le mode est activé.
#define MODE_STATS_ENABLED       0b10000000
#define MODE_COMPILE_ENABLED     0b01000000
#define MODE_LINK_ENABLED        0b00100000
#define MODE_CLEAN_ENABLED       0b00010000

// TODO: faire ces 2 modes :
#define MODE_EXEC_ENABLED        0b00001000
#define MODE_LINES_COUNT_ENABLED 0b00000100

#define MODE_ALL                 0b11100000
#define MODE_LINK                0b10100000
#define MODE_REBUILD             0b11100001
#define MODE_EXEC                0b00001000
#define MODE_LINES_COUNT         0b00000100

#define C_LANGUAGE   1
#define CPP_LANGUAGE 2

// Numéro de version du programme.
extern const uchar *VERSION;
// Nom du programme.
extern const uchar *PROGRAM_NAME;
// Nom du fichier de configuration (Cakefile).
extern const uchar *OPTIONS_FILENAME;

extern const uchar *STDERR;
extern const uchar *STDOUT;
extern const uchar *STDIN;

/*
        Langage de programmation à compiler.

        Liste :
        - C
        - C++
*/
extern cake_byte g_ModeLanguage;

// Détermine si le programme s'exécute automatiquement après la compilation.
extern cake_bool g_AutoExec;
extern cake_byte g_Mode;

extern ulonglong g_NeedCompileNumber;
extern ulonglong g_CompileNumber;
extern ulonglong g_CurrentCompile;

extern cake_bool g_Quiet;

// Chemin actuel.
extern Cake_String_UTF8 *g_CurrentDir;

extern Cake_FileObjectElement *o_ProgrammingLanguage;
extern Cake_FileObjectElement *o_ObjDir;
extern Cake_FileObjectElement *o_BinDir;
extern Cake_FileObjectElement *o_Compiler;
extern Cake_FileObjectElement *o_ExecName;
extern Cake_List_String_UTF8  *o_Includes;
extern Cake_List_String_UTF8  *o_Libs;
extern Cake_FileObjectElement *o_CompileOptions;
extern Cake_FileObjectElement *o_LinkOptions;
extern Cake_FileObjectElement *o_LinkLibs;
extern Cake_FileObjectElement *o_AutoExec;
extern Cake_FileObjectElement *o_ExecArgs;

#endif