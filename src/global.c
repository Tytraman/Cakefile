#include "../include/global.h"

const char *VERSION             =  "2.2.0";
const char *PROGRAM_NAME        =  "Cake";
const wchar_t *OPTIONS_FILENAME = L"Cakefile";

const char *STDERR = "STDERR";
const char *STDOUT = "STDOUT";
const char *STDIN  = "STDIN";

const wchar_t FILE_SEPARATOR =
    #ifdef _WIN32
    L'\\';
    #else
    L'/';
    #endif

const wchar_t REVERSE_FILE_SEPARATOR =
    #ifdef _WIN32
    L'/';
    #else
    L'\\';
    #endif

char g_ModeLanguage = 0;

char g_Mode = MODE_ALL;
BOOL g_AutoExec = FALSE;

String_UTF16 g_ProgramFilename;
String_UTF16 g_Pwd;

Option o_Language;
Option o_ObjDir;
Option o_BinDir;
Option o_Includes;
Option o_Libs;
Option o_CompileOptions;
Option o_LinkOptions;
Option o_LinkLibs;
Option o_ExecName;
Option o_AutoExec;
Option o_ExecArgs;

String_UTF16 g_Compiler;
