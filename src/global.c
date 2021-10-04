#include "../include/global.h"

const char *VERSION      = "2.0.0";
const char *PROGRAM_NAME = "Cake";

const char *STDERR = "STDERR";
const char *STDOUT = "STDOUT";
const char *STDIN  = "STDIN";

char mode = MODE_ALL;

String_UTF16 programFilename;
String_UTF16 pwd;

char programStatus = 0;

HANDLE stdoutParent;
HANDLE stderrParent;

String_UTF16 srcDir;
String_UTF16 objDir;
String_UTF16 binDir;

String_UTF16 includes;
String_UTF16 libs;

String_UTF16 compileOptions;
String_UTF16 linkOptions;
String_UTF16 linkLibs;

String_UTF16 exec;