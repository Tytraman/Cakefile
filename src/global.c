#include "../include/global.h"

const char *VERSION      = "1.0.0";
const char *PROGRAM_NAME = "Cake";

DWORD programFilenameLength;
char *programFilename = NULL;

char programStatus = 0;

HANDLE stdoutRead, stdoutWrite;
HANDLE stderrRead, stderrWrite;
HANDLE stdoutParent;
HANDLE stderrParent;
