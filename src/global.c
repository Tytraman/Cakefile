#include "../include/global.h"
#include "../include/utils.h"

const char *VERSION      = "1.0.0";
const char *PROGRAM_NAME = "Cake";

Array_Char programFilename;
Array_Char pwd;

char programStatus = 0;

HANDLE stdoutRead, stdoutWrite;
HANDLE stderrRead, stderrWrite;
HANDLE stdoutParent;
HANDLE stderrParent;

Array_Char srcDir;
Array_Char objDir;
Array_Char binDir;

Array_Char includes;
Array_Char libs;

Array_Char compileOptions;
Array_Char linkOptions;
Array_Char linkLibs;

Array_Char exec;