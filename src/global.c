#include "../include/global.h"
#include "../include/utils.h"

const char *VERSION      = "1.0.0";
const char *PROGRAM_NAME = "Cake";

char programStatus = 0;

HANDLE stdoutRead, stdoutWrite;
HANDLE stderrRead, stderrWrite;
HANDLE stdoutParent;
HANDLE stderrParent;

long srcDirLength     = 0UL;
unsigned char *srcDir = NULL;

long objDirLength     = 0UL;
unsigned char *objDir = NULL;

long binDirLength     = 0UL;
unsigned char *binDir = NULL;

long includesLength     = 0UL;
unsigned char *includes = NULL;

long libsLength     = 0UL;
unsigned char *libs = NULL;

long compileOptionsLength = 0UL;
unsigned char *compileOptions = NULL;

long linkOptionsLength     = 0UL;
unsigned char *linkOptions = NULL;

Array_Char exec;

Array_Char programFilename;
Array_Char pwd;