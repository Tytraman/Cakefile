#include "../include/global.h"

const char *VERSION             =  "2.1.1";
const char *PROGRAM_NAME        =  "Cake";
const wchar_t *OPTIONS_FILENAME = L"Cakefile";

const char *STDERR = "STDERR";
const char *STDOUT = "STDOUT";
const char *STDIN  = "STDIN";

char modeLanguage = 0;

char mode = MODE_ALL;

String_UTF16 programFilename;
String_UTF16 pwd;

char programStatus = 0;

Option o_language;
Option o_objDir;
Option o_binDir;
Option o_includes;
Option o_libs;
Option o_compileOptions;
Option o_linkOptions;
Option o_linkLibs;
Option o_execName;

String_UTF16 compiler;
