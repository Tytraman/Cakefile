#include "../include/global.h"
#include "../include/utils.h"

const char *VERSION      = "1.0.2";
const char *PROGRAM_NAME = "Cake";

const char *STDERR = "STDERR";
const char *STDOUT = "STDOUT";
const char *STDIN  = "STDIN";

Array_Char programFilename;
Array_Char pwd;

char programStatus = 0;

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