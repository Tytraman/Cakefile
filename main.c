#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <windows.h>

#include "include/utils.h"
#include "include/global.h"
#include "include/error.h"


int main(int argc, char **argv) {
    HWND consoleWnd = GetConsoleWindow();
    DWORD processID;
    GetWindowThreadProcessId(consoleWnd, &processID);
    if(GetCurrentProcessId() == processID) {
        FreeConsole();
        MessageBoxW(NULL, L"Le programme doit être exécuté depuis un terminal.", L"Erreur !", MB_OK | MB_ICONERROR);
        return PROGRAM_STATUS_DOUBLE_CLICKS;
    }
    
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);

    if(argc > 1) {
        if(strcmp(argv[1], "--help") == 0) {
            wprintf(
                L"==========%S==========\n"
                L"Le compilateur utilisé est gcc, alors assure toi de l'avoir installé et d'avoir la variable d'environnement !\n"
                L"Les options suivantes doivent être mises dans un fichier `Cakefile` (sans extension).\n"
                L"Le format est clé : valeur\n\n"
                L"Liste des options :\n"
                L"[ Obligatoires ]\n"
                L"- exec_name : nom du fichier exécutable final.\n"
                L"- obj_dir : dossier dans lequel les fichiers compilés logeront.\n"
                L"- bin_dir : dossier dans lequel le fichier exécutable prendra ses aises.\n\n"
                L"[ Optionnelles ]\n"
                L"- includes : Liste des dossiers includes externes à inclure.\n"
                L"- libs : Liste des dossiers de librairies externes à inclure.\n"
                L"- compile_options : options utilisées pendant la compilation.\n"
                L"- link_options : options utilisées pendant le link des fichiers objets.\n"
                L"========================\n"
                , PROGRAM_NAME
            );
            return 0;
        }else if(strcmp(argv[1], "--version") == 0) {
            wprintf(L"%Sx%S version %S\n", PROGRAM_NAME, (sizeof(void *) == 8 ? "64" : "86"), VERSION);
            return 0;
        }
    }

    FILE *pFile = fopen("Cakefile", "rb");
    if(!pFile) {
        error_file_not_found("Cakefile");
        return PROGRAM_STATUS_CAKEFILE_NOT_FOUND;
    }
    
    // Copie du fichier de configuration :
    unsigned char buff[BUFF_SIZE];

    long fileSize = get_file_size(pFile);
    unsigned char *fileBuffer = malloc(fileSize + 1);
    size_t read, total = 0;

    do {
        read = fread(buff, 1, BUFF_SIZE, pFile);
        memcpy(&fileBuffer[total], buff, read);
        total += read;
    }while(read > 0);

    fileBuffer[fileSize] = '\0';

    fclose(pFile);

    // Récupération des paramètres :
    unsigned char *temp;

    long objDirSize;
    unsigned char *objDir = NULL;
    long binDirSize;
    unsigned char *binDir = NULL;

    long execNameSize;
    unsigned char *execName = NULL;

    long includesSize;
    unsigned char *includes = NULL;
    long libsSize;
    unsigned char *libs = NULL;

    long compileOptionsSize;
    unsigned char *compileOptions = NULL;
    long linkOptionsSize;
    unsigned char *linkOptions = NULL;

    if((temp = get_key_value("obj_dir", fileBuffer, fileSize, &objDirSize)))
        copy_value(&objDir, temp, objDirSize);
    else {
        error_key_not_found("obj_dir");
        goto program_end;
    }

    if((temp = get_key_value("bin_dir", fileBuffer, fileSize, &binDirSize)))
        copy_value(&binDir, temp, binDirSize);
    else {
        error_key_not_found("bin_dir");
        goto program_end;
    }

    if((temp = get_key_value("exec_name", fileBuffer, fileSize, &execNameSize)))
        copy_value(&execName, temp, execNameSize);
    else {
        error_key_not_found("exec_name");
        goto program_end;
    }

    if((temp = get_key_value("includes", fileBuffer, fileSize, &includesSize)))
        copy_value(&includes, temp, includesSize);
    else
        empty_str(&includes);

    if((temp = get_key_value("libs", fileBuffer, fileSize, &libsSize)))
        copy_value(&libs, temp, libsSize);
    else
        empty_str(&libs);

    if((temp = get_key_value("compile_options", fileBuffer, fileSize, &compileOptionsSize)))
        copy_value(&compileOptions, temp, compileOptionsSize);
    else
        empty_str(&compileOptions);

    if((temp = get_key_value("link_options", fileBuffer, fileSize, &linkOptionsSize)))
        copy_value(&linkOptions, temp, linkOptionsSize);
    else
        empty_str(&linkOptions);

    stdoutParent = GetStdHandle(STD_OUTPUT_HANDLE);
    stderrParent = GetStdHandle(STD_ERROR_HANDLE);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if(!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        error_create_pipe(GetLastError());
        goto program_end;
    }
    if(!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        error_create_pipe(GetLastError());
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        goto program_end;
    }
    if(!SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0)) {
        error_set_handle_infos(GetLastError());
        goto close_handles;
    }
    if(!SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0)) {
        error_set_handle_infos(GetLastError());
        goto close_handles;
    }

    Array_Char dirCout, dirCerr;
    Array_Char dirHout, dirHerr;

    char dirC[] = "cmd /C dir *.c /b/s";
    char result;
    if((result = execute_command(dirC, &dirCout, &dirCerr)) != 0) {
        if(result == 1)
            error_create_process(dirC, GetLastError());
        else {
            free(dirCout.array);
            free(dirCerr.array);
            dirCout.array = NULL;
            dirCerr.array = NULL;
        }
    }
    dirC[13] = 'h';
    if((result = execute_command(dirC, &dirHout, &dirHerr)) != 0) {
        if(result == 1)
            error_create_process(dirC, GetLastError());
        else {
            free(dirHout.array);
            free(dirHerr.array);
            dirHout.array = NULL;
            dirHerr.array = NULL;
        }
    }

    Array_Char **listC = NULL;
    unsigned long listCsize = list_files(&listC, &dirCout);

close_handles:
    CloseHandle(stdoutRead);
    CloseHandle(stdoutWrite);
    CloseHandle(stderrRead);
    CloseHandle(stderrWrite);
program_end:
    free(objDir);
    free(binDir);
    free(execName);
    free(fileBuffer);
    free(includes);
    free(libs);
    free(compileOptions);
    free(linkOptions);
    return programStatus;
}
