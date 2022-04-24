#include "global.h"

cchar_ptr VERSION          = "2.3.0";
cchar_ptr PROGRAM_NAME     = "Cakefile";
cchar_ptr OPTIONS_FILENAME = "Cakefile";

cchar_ptr STDERR = "STDERR";
cchar_ptr STDOUT = "STDOUT";
cchar_ptr STDIN  = "STDIN";

cake_byte g_ModeLanguage = 0;

cake_byte g_Mode = MODE_ALL;

ulonglong g_NeedCompileNumber = 0;
ulonglong g_CompileNumber = 0;
ulonglong g_CurrentCompile = 0;

Cake_String_UTF8 *g_CurrentDir = NULL;

cake_bool g_Quiet = cake_false;

Cake_FileObjectElement *o_ProgrammingLanguage = NULL;
Cake_FileObjectElement *o_ObjDir = NULL;
Cake_FileObjectElement *o_BinDir = NULL;
Cake_FileObjectElement *o_Compiler = NULL;
Cake_List_String_UTF8  *o_Includes = NULL;
Cake_List_String_UTF8  *o_Libs = NULL;
Cake_FileObjectElement *o_CompileOptions = NULL;
Cake_FileObjectElement *o_LinkOptions = NULL;
Cake_FileObjectElement *o_LinkLibs = NULL;
Cake_FileObjectElement *o_ExecName = NULL;
Cake_FileObjectElement *o_AutoExec = NULL;
Cake_FileObjectElement *o_ExecArgs = NULL;
