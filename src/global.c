#include "../include/global.h"

const char *VERSION             =  "2.3.0";
const char *PROGRAM_NAME        =  "Cake";
const wchar_t *OPTIONS_FILENAME = L"Cakefile";

HANDLE g_Out = NULL;
CONSOLE_SCREEN_BUFFER_INFO g_ScreenInfo = { 0 };

short g_LastX = 0;
short g_LastY = 0;
char g_DrawProgressBar = 0;
short g_ProgressBarWidthScale = 50;
wchar_t g_ProgressBarEmptyChar = L' ';
wchar_t g_ProgressBarFillChar  = 0x2588;

const char *STDERR = "STDERR";
const char *STDOUT = "STDOUT";
const char *STDIN  = "STDIN";

char g_ModeLanguage = 0;

unsigned int g_Mode = MODE_ALL;
BOOL g_AutoExec = FALSE;

unsigned long g_NeedCompileNumber = 0;
unsigned long g_CompileNumber = 0;
unsigned long g_CurrentCompile = 0;

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
