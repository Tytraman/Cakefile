#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <windows.h>

#include "include/utils.h"
#include "include/global.h"
#include "include/error.h"

#define MODE_ALL   1
#define MODE_RESET 2
#define MODE_LINK  3
#define MODE_CLEAN 4

int main(int argc, char **argv) {
    HWND consoleWnd = GetConsoleWindow();
    DWORD processID;
    GetWindowThreadProcessId(consoleWnd, &processID);
    if(GetCurrentProcessId() == processID) {
        FreeConsole();
        MessageBoxW(NULL, L"Le programme doit être exécuté depuis un terminal.", L"Erreur !", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);

    char mode = MODE_ALL;

    if(argc > 1) {
        if(strcmp(argv[1], "all") == 0)
            mode = MODE_ALL;
        else if(strcmp(argv[1], "reset") == 0)
            mode = MODE_RESET;
        else if(strcmp(argv[1], "link") == 0)
            mode = MODE_LINK;
        else if(strcmp(argv[1], "clean") == 0)
            mode = MODE_CLEAN;
        else if(strcmp(argv[1], "--help") == 0) {
            wprintf(
                L"==========%S==========\n"
                L"Le compilateur utilisé est gcc, alors assure toi de l'avoir installé et d'avoir la variable d'environnement !\n"
                L"Les options suivantes doivent être mises dans un fichier `Cakefile` (sans extension).\n"
                L"Le format est clé : valeur\n\n"
                L"Liste des options :\n"
                L"[ Obligatoires ]\n"
                L"- exec_name : nom du fichier exécutable final.\n"
                L"- src_dir : dossier où se situent les fichiers sources.\n"
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
            return EXIT_SUCCESS;
        }else if(strcmp(argv[1], "--version") == 0) {
            wprintf(L"%Sx%S version %S\n", PROGRAM_NAME, (sizeof(void *) == 8 ? "64" : "86"), VERSION);
            return EXIT_SUCCESS;
        }
    }

    programFilenameLength = get_program_file_name(&programFilename);

    FILE *pFile = fopen("Cakefile", "rb");
    if(!pFile) {
        error_file_not_found("Cakefile");
        return EXIT_FAILURE;
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

    long srcDirSize;
    unsigned char *srcDir = NULL;
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

    char key1[] = "src_dir";
    char key2[] = "obj_dir";
    char key3[] = "bin_dir";
    char key4[] = "exec_name";
    char key5[] = "includes";
    char key6[] = "libs";
    char key7[] = "compile_options";
    char key8[] = "link_options";

    if((temp = get_key_value(key1, fileBuffer, fileSize, &srcDirSize)))
        copy_value(&srcDir, temp, srcDirSize);
    else {
        error_key_not_found(key1);
        goto program_end;
    }

    
    if((temp = get_key_value(key2, fileBuffer, fileSize, &objDirSize)))
        copy_value(&objDir, temp, objDirSize);
    else {
        error_key_not_found(key2);
        goto program_end;
    }

    if((temp = get_key_value(key3, fileBuffer, fileSize, &binDirSize)))
        copy_value(&binDir, temp, binDirSize);
    else {
        error_key_not_found(key3);
        goto program_end;
    }

    if((temp = get_key_value(key4, fileBuffer, fileSize, &execNameSize)))
        copy_value(&execName, temp, execNameSize);
    else {
        error_key_not_found(key4);
        goto program_end;
    }

    if((temp = get_key_value(key5, fileBuffer, fileSize, &includesSize)))
        copy_value(&includes, temp, includesSize);
    else
        empty_str(&includes);

    if((temp = get_key_value(key6, fileBuffer, fileSize, &libsSize)))
        copy_value(&libs, temp, libsSize);
    else
        empty_str(&libs);

    if((temp = get_key_value(key7, fileBuffer, fileSize, &compileOptionsSize)))
        copy_value(&compileOptions, temp, compileOptionsSize);
    else
        empty_str(&compileOptions);

    if((temp = get_key_value(key8, fileBuffer, fileSize, &linkOptionsSize)))
        copy_value(&linkOptions, temp, linkOptionsSize);
    else
        empty_str(&linkOptions);

    stdoutParent = GetStdHandle(STD_OUTPUT_HANDLE);
    stderrParent = GetStdHandle(STD_ERROR_HANDLE);


    // Création des tunnels de redirection :
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
    dirCout.array = NULL; dirCerr.array = NULL;
    dirHout.array = NULL; dirHerr.array = NULL;

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

    /* Le nom de l'exe est nécessaire dans tous les modes */
    Array_Char exe;
    exe.length = binDirSize + execNameSize + 1;
    exe.array = malloc(exe.length);
    memcpy(exe.array, binDir, binDirSize);
    exe.array[binDirSize] = '\\';
    memcpy(&exe.array[binDirSize + 1], execName, execNameSize);
    exe.array[exe.length] = '\0';
    free(execName);
    execName = NULL;

    /* La liste des fichiers C est nécessaire que pour compiler */
    Array_Char **listC = NULL;
    unsigned long listCsize = 0UL;

    /* La liste des fichiers H est nécessaire que pour compiler */
    Array_Char **listH = NULL;
    unsigned long listHsize = 0UL;

    /* La liste des fichiers O est nécessaire pour linker */
    Array_Char **listO = NULL;
    unsigned long listOsize = 0UL;

    if(mode == MODE_ALL || mode == MODE_RESET || mode == MODE_LINK)
        if(dirCout.array)
            listOsize = list_o_files(&listO, &dirCout, srcDir, srcDirSize, objDir, objDirSize);
    
    // Listing des fichiers C et H
    if(mode == MODE_ALL || mode == MODE_RESET) {
        if(dirCout.array) {
            listCsize = list_files(&listC, &dirCout);
            free(dirCout.array);
            free(dirCerr.array);
            dirCout.array = NULL;
            dirCerr.array = NULL;
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
        if(dirHout.array) {
            listHsize = list_files(&listH, &dirHout);
            free(dirHout.array);
            free(dirHerr.array);
            dirHout.array = NULL;
            dirHerr.array = NULL;
        }
    }
    
    // Création des dossiers nécessaires
    if(mode == MODE_ALL || mode == MODE_RESET) {
        if((result = mkdirs(objDir)) != 0)
            if(result == 2) error_create_folder(objDir);

        if((result = mkdirs(binDir)) != 0)
            if(result == 2) error_create_folder(binDir);
    }else if(mode == MODE_LINK)
        if((result = mkdirs(binDir)) != 0)
            if(result == 2) error_create_folder(binDir);
    
pre_end1:
    // Libération de la mémoire des listes de fichiers
    if(listC)
        free_list(&listC, listCsize);
    if(listH)
        free_list(&listH, listHsize);


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
    free(programFilename);
    return (programStatus != 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
