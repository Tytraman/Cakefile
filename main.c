#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "include/global.h"
#include "include/error.h"
#include "include/encoding/utf8.h"
#include "include/encoding/utf16.h"
#include "include/file/file_utils.h"
#include "include/memory/memory.h"
#include "include/os/winapi.h"


char check_args(int argc, char **argv);
unsigned long list_files(String_UTF16 ***listDest, String_UTF16 *src);
unsigned long list_o_files(String_UTF16 ***listDest, String_UTF16 *src);
unsigned long check_who_must_compile(unsigned long **list, String_UTF16 **listO, String_UTF16 **listC, unsigned long listOsize);
void check_includes(unsigned long **list, unsigned long *listSize, unsigned long current, String_UTF16 *fileC, String_UTF16 *fileO);
char create_object(String_UTF16 *cFile, String_UTF16 *oFile, char *error);
void combine_includes_path();
void combine_libs_path();
char load_options();
void unload_options();
char selectCompiler();
void free_all();

char check_args(int argc, char **argv) {
    if(argc > 1) {
        if(strcasecmp(argv[1], "all") == 0)
            g_Mode = MODE_ALL;
        else if(strcasecmp(argv[1], "reset") == 0)
            g_Mode = MODE_RESET;
        else if(strcasecmp(argv[1], "link") == 0)
            g_Mode = MODE_LINK;
        else if(strcasecmp(argv[1], "clean") == 0)
            g_Mode = MODE_CLEAN;
        else if(strcasecmp(argv[1], "--help") == 0) {
            wprintf(
                L"==========[ %S ]==========\n"
                L"Lorsqu'aucun argument n'est passé, la commande est équivalente à `cake all`.\n"
                L"[ Arguments ]\n"
                L"> clean : supprime tous les fichiers objets et l'exécutable.\n"
                L"> all : compile les fichiers modifiés puis crée l'exécutable.\n"
                L"> reset : équivalent de `cake clean` puis `cake all`.\n"
                L"> --help : affiche ce message.\n"
                L"> --version : affiche la version installée du programme.\n"
                L"> --generate : génère un fichier `Cakefile` avec les options par défaut.\n\n"
                
                L"Liste des options du fichier `Cakefile` :\n"
                L"[ Obligatoires ]\n"
                L"- language : langage de programmation utilisé.\n"
                L"- obj_dir : dossier où sont stockés les fichiers `.o` une fois les fichiers `.c` compilés.\n"
                L"- bin_dir : dossier où sera stocké l'exécutable final.\n"
                L"- exec_name : nom de l'exécutable final.\n\n"

                L"[ Optionnelles ]\n"
                L"- includes : liste des dossiers includes externes à inclure.\n"
                L"- libs : liste des dossiers de librairies externes à inclure.\n"
                L"- compile_options : options utilisées pendant la compilation.\n"
                L"- link_options : options utilisées pendant le link des fichiers objets.\n"
                L"- link_l : librairies externes à inclure.\n"
                L"- auto_exec : définie si le programme s'exécute automatiquement après sa génération. (true/false)\n"
                L"- exec_args : liste des arguments à passer si auto_exec est activé ou si la commande `cake exec` a été tapée.\n\n"
                
                L"Pour plus d'infos, voir la page github : https://github.com/Tytraman/Cakefile\n"
                L"============================\n"
                , PROGRAM_NAME
            );
            return 0;
        }else if(strcasecmp(argv[1], "--version") == 0) {
            wprintf(L"%S x%S version %S\n", PROGRAM_NAME, (sizeof(void *) == 8 ? "64" : "86"), VERSION);
            return 0;
        }else if(strcasecmp(argv[1], "--generate") == 0) {
            if(GetFileAttributesW(OPTIONS_FILENAME) == 0xffffffff) {
                unsigned char defaultCakefile[] =
                    "language : c\n\n"
                    "obj_dir : obj\n"
                    "bin_dir : bin\n\n"

                    "exec_name : prog.exe\n\n"

                    "includes : \n"
                    "libs : \n\n"

                    "compile_options : \n"
                    "link_options : \n\n"

                    "link_l : \n\n"
                    "auto_exec : false\n"
                    "exec_args :";
                FILE *pCakefile = _wfopen(OPTIONS_FILENAME, L"wb");
                fwrite(defaultCakefile, 1, 162, pCakefile);
                fclose(pCakefile);
            }else
                fwprintf(stderr, L"[%S] Il existe déjà un fichier `%s`.\n", PROGRAM_NAME, OPTIONS_FILENAME);
            return 0;
        }else {
            fwprintf(stderr, L"[%S] Argument invalide, entre `cake --help` pour afficher l'aide.", PROGRAM_NAME);
            return 0;
        }
    }
    return 1;
}

unsigned long list_files(String_UTF16 ***listDest, String_UTF16 *src) {
    unsigned long size = 0;
    *listDest = NULL;

    wchar_t *ptr = src->characteres;
    unsigned long i;
    for(i = 0; i < src->length; i++) {
        if(src->characteres[i] == L'\r') {
            *listDest = (String_UTF16 **) realloc(*listDest, (size + 1) * sizeof(String_UTF16 *));
            (*listDest)[size] = (String_UTF16 *) malloc(sizeof(String_UTF16));
            create_string_utf16((*listDest)[size]);
            src->characteres[i] = L'\0';
            string_utf16_set_value((*listDest)[size], ptr);
            string_utf16_remove((*listDest)[size], pwd.characteres);
            size++;
            i++;
            ptr = &src->characteres[i + 1];
        }
    }

    return size;
}

unsigned long list_o_files(String_UTF16 ***listDest, String_UTF16 *src) {
    // Il faut copier la source dans un tampon pour éviter de la modifier
    String_UTF16 srcCopy;
    create_string_utf16(&srcCopy);
    string_utf16_set_value(&srcCopy, src->characteres);

    // Création d'une liste des fichiers sources
    unsigned long number = list_files(listDest, &srcCopy);
    free(srcCopy.characteres);

    // Dossier obj à ajouter devant le chemin de chaque fichier n'ayant pas le dossier src
    String_UTF16 insertObj;
    create_string_utf16(&insertObj);
    string_utf16_set_value(&insertObj, o_ObjDir.value.characteres);
    string_utf16_add_char(&insertObj, L'\\');  // obj\ 

    // Remplacement de l'extension par .o
    // puis
    // Ajout du dossier obj
    unsigned long i;
    for(i = 0; i < number; i++) {
        switch(g_ModeLanguage) {
            case C_LANGUAGE:
                strutf16_replace_from_end((*listDest)[i], L".c", L".o");
                break;
            case CPP_LANGUAGE:
                if(!strutf16_replace_from_end((*listDest)[i], L".cpp", L".o"))
                    if(!strutf16_replace_from_end((*listDest)[i], L".c++", L".o"))
                        strutf16_replace_from_end((*listDest)[i], L".c", L".o");
                break;
            default:
                break;
        }
        /*
                Ancien code.

                Avant, le dossier source était remplacé par le dossier objet, mais
                ça créait un problème facilement patchable, mais après réfléxion, je trouve
                ce principe un peu nul puisqu'il nécessite de suivre une directive bien précise.

                Le problème était que, le remplacement se produisait comme ceci :
                    [ok] src\test\oui.o    -> obj\test\oui.o
                    [ok] sub\folder\argh.o -> obj\sub\folder\non.o
                    [euh ?] dir\src\hmm.o  -> dir\obj\hmm.o
                
                src était remplacé par obj sans vérifier que src soit le premier dossier.
        */
        /*
        if(!string_utf16_replace((*listDest)[i], o_srcDir.value.characteres, o_objDir.value.characteres))
            string_utf16_insert((*listDest)[i], insertObj.characteres);
        */

        if(!strutf16_start_with((*listDest)[i], o_ObjDir.value.characteres))
            string_utf16_insert((*listDest)[i], insertObj.characteres);
    }

    free(insertObj.characteres);
    return number;
}

unsigned long check_who_must_compile(unsigned long **list, String_UTF16 **listO, String_UTF16 **listC, unsigned long listOsize) {
    unsigned long number = 0UL;
    unsigned long i;
    FILETIME lastModifiedO, lastModifiedC;
    HANDLE hFileO, hFileC;

    for(i = 0UL; i < listOsize; i++) {
        if(!file_exists(listO[i]->characteres)) {
            *list = (unsigned long *) realloc(*list, (number + 1) * sizeof(unsigned long));
            (*list)[number++] = i;
        }else {
            if((hFileO = CreateFileW(listO[i]->characteres, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
                if((hFileC = CreateFileW(listC[i]->characteres, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
                    GetFileTime(hFileO, NULL, NULL, &lastModifiedO);
                    GetFileTime(hFileC, NULL, NULL, &lastModifiedC);
                    if(CompareFileTime(&lastModifiedO, &lastModifiedC) == -1) {
                        CloseHandle(hFileO);
                        CloseHandle(hFileC);
                        *list = (unsigned long *) realloc(*list, (number + 1) * sizeof(unsigned long));
                        (*list)[number++] = i;
                    }else {
                        CloseHandle(hFileO);
                        CloseHandle(hFileC);
                        check_includes(list, &number, i, listC[i], listO[i]);
                    }
                }else {
                    CloseHandle(hFileO);
                    error_open_file(listC[i]->characteres);
                }
            }else
                error_open_file(listO[i]->characteres);
        }
    }
    return number;
}

void check_includes(unsigned long **list, unsigned long *listSize, unsigned long current, String_UTF16 *fileC, String_UTF16 *fileO) {
    String_UTF8 fileUtf8;
    create_string_utf8(&fileUtf8);
    if(!open_utf8_file(&fileUtf8, fileC->characteres)) {
        free(fileUtf8.bytes);
        return;
    }
    String_UTF16 fileUtf16;
    create_string_utf16(&fileUtf16);
    string_utf8_to_utf16(&fileUtf8, &fileUtf16);
    free(fileUtf8.bytes);

    String_UTF16 **listI = NULL;
    unsigned long listIsize = 0;
    unsigned long i;

    const wchar_t *search = L"#include";

    wchar_t *ptr;
    unsigned long index = 0;
    while((ptr = string_utf16_search_from(&fileUtf16, (wchar_t *) search, &index)) != NULL) {

        // On cherche le premier '\"'
        while(fileUtf16.characteres[index] != L'\"')
            if(index == fileUtf16.length || fileUtf16.characteres[index] == L'\r' || fileUtf16.characteres[index++] == L'\n')
                goto end_loop_search_includes;

        ptr = &fileUtf16.characteres[++index];

        // On cherche le deuxième '\"'
        while(fileUtf16.characteres[index] != L'\"')
            if(index == fileUtf16.length || fileUtf16.characteres[index] == L'\r' || fileUtf16.characteres[index++] == L'\n')
                goto end_loop_search_includes;

        // On récupère le nom du fichier include
        listI = (String_UTF16 **) realloc(listI, (listIsize + 1) * sizeof(String_UTF16 *));
        listI[listIsize] = (String_UTF16 *) malloc(sizeof(String_UTF16));
        listI[listIsize]->length = &fileUtf16.characteres[index] - ptr;
        listI[listIsize]->characteres = (wchar_t *) malloc(listI[listIsize]->length * sizeof(wchar_t) + sizeof(wchar_t));
        memcpy(listI[listIsize]->characteres, ptr, listI[listIsize]->length * sizeof(wchar_t));
        listI[listIsize]->characteres[listI[listIsize]->length] = L'\0';
        listIsize++;

        end_loop_search_includes: ;
    }


    // On vérifie si le fichier nécessite d'être recompilé
    for(i = 0; i < listIsize; i++) {
        string_utf16_replace_all_char(listI[i], L'/', L'\\');

        String_UTF16 fullPath;
        string_utf16_copy(fileC, &fullPath);
        if(!string_utf16_remove_part_from_end(&fullPath, L'\\'))
            string_utf16_empty(&fullPath);
        else
            string_utf16_add_char(&fullPath, L'\\');
        string_utf16_add(&fullPath, listI[i]->characteres);

        HANDLE hFileO, hFileH;
        if((hFileH = CreateFileW(fullPath.characteres, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
            if((hFileO = CreateFileW(fileO->characteres, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
                FILETIME lastModifiedO, lastModifiedH;
                GetFileTime(hFileO, NULL, NULL, &lastModifiedO);
                GetFileTime(hFileH, NULL, NULL, &lastModifiedH);
                if(CompareFileTime(&lastModifiedO, &lastModifiedH) == -1) {
                    *list = (unsigned long *) realloc(*list, (*listSize + 1) * sizeof(unsigned long));
                    (*list)[(*listSize)++] = current;
                }
                CloseHandle(hFileO);
                CloseHandle(hFileH);
            }else {
                CloseHandle(hFileH);
                error_open_file(fileO->characteres);
            }
        }else
            error_open_file(fullPath.characteres);

        free(fullPath.characteres);
    }
}

char create_object(String_UTF16 *cFile, String_UTF16 *oFile, char *error) {
    String_UTF16 command;

    string_utf16_copy(&compiler, &command);

    // gcc -c "
    string_utf16_add(&command, L" -c \"");

    // gcc -c "fichier.c
    string_utf16_add(&command, cFile->characteres);

    // gcc -c "fichier.c" -o "
    string_utf16_add(&command, L"\" -o \"");

    // gcc -c "fichier.c" -o "fichier.o
    string_utf16_add(&command, oFile->characteres);

    // gcc -c "fichier.c" -o "fichier.o"
    string_utf16_add_char(&command, L'\"');

    if(o_CompileOptions.loaded) {
        string_utf16_add_char(&command, L' ');
        string_utf16_add(&command, o_CompileOptions.value.characteres);
    }

    if(o_Includes.loaded)
        string_utf16_add(&command, o_Includes.value.characteres);

    if(o_Libs.loaded)
        string_utf16_add(&command, o_Libs.value.characteres);

    wprintf(L"%s\n", command.characteres);

    char result;
    if((result = execute_command(command.characteres, NULL, NULL, error, 1, 1)) != 0)
        error_execute_command(command.characteres, result);
    free(command.characteres);
    return result;
}

void combine_includes_path() {
    unsigned long begin = 0, end = 0;
                    
    String_UTF16 copy, temp;
    string_utf16_copy(&o_Includes.value, &copy);
    string_utf16_empty(&o_Includes.value);

    while(string_utf16_find(&copy, L'|', &end)) {
        string_utf16_copy_between(&copy, &temp, begin, end);
        string_utf16_rtrim(&temp, L'\\');
        end++;
        string_utf16_add(&o_Includes.value, L" -I \"");
        string_utf16_add(&o_Includes.value, temp.characteres);
        string_utf16_add_char(&o_Includes.value, L'\"');
        free(temp.characteres);
        begin = end;
    }

    string_utf16_copy_between(&copy, &temp, begin, copy.length);
    string_utf16_rtrim(&temp, L'\\');
    string_utf16_add(&o_Includes.value, L" -I \"");
    string_utf16_add(&o_Includes.value, temp.characteres);
    string_utf16_add_char(&o_Includes.value, L'\"');
    free(temp.characteres);
    free(copy.characteres);
}

void combine_libs_path() {
    unsigned long begin = 0, end = 0;
                    
    String_UTF16 copy, temp;
    string_utf16_copy(&o_Libs.value, &copy);
    string_utf16_empty(&o_Libs.value);

    while(string_utf16_find(&copy, L'|', &end)) {
        string_utf16_copy_between(&copy, &temp, begin, end);
        string_utf16_rtrim(&temp, L'\\');
        end++;
        string_utf16_add(&o_Libs.value, L" -L \"");
        string_utf16_add(&o_Libs.value, temp.characteres);
        string_utf16_add_char(&o_Libs.value, L'\"');
        free(temp.characteres);
        begin = end;
    }

    string_utf16_copy_between(&copy, &temp, begin, copy.length);
    string_utf16_rtrim(&temp, L'\\');
    string_utf16_add(&o_Libs.value, L" -L \"");
    string_utf16_add(&o_Libs.value, temp.characteres);
    string_utf16_add_char(&o_Libs.value, L'\"');
    free(temp.characteres);
    free(copy.characteres);
}

char load_options(String_UTF16 *from) {
    init_option(&o_ObjDir);
    init_option(&o_BinDir);
    init_option(&o_Includes);
    init_option(&o_Libs);
    init_option(&o_CompileOptions);
    init_option(&o_LinkOptions);
    init_option(&o_LinkLibs);
    init_option(&o_ExecName);
    init_option(&o_AutoExec);
    init_option(&o_ExecArgs);

    const wchar_t *key_Language = L"language";
    const wchar_t *key_SrcDir   = L"src_dir";
    const wchar_t *key_ObjDir   = L"obj_dir";
    const wchar_t *key_BinDir   = L"bin_dir";
    const wchar_t *key_ExecName = L"exec_name";
    const wchar_t *key_AutoExec = L"auto_exec";
    const wchar_t *key_ExecArgs = L"exec_args";

    if(!load_option(key_Language, from, &o_Language)) {
        error_key_not_found(key_Language);
        return 1;
    }

    if(!load_option(key_ObjDir, from, &o_ObjDir)) {
        error_key_not_found(key_ObjDir);
        return 2;
    }

    if(!load_option(key_BinDir, from, &o_BinDir)) {
        error_key_not_found(key_BinDir);
        return 3;
    }

    if(!load_option(key_ExecName, from, &o_ExecName)) {
        error_key_not_found(key_ExecName);
        return 4;
    }

    if(load_option(L"includes", from, &o_Includes))
        combine_includes_path();

    if(load_option(L"libs", from, &o_Libs))
        combine_libs_path();

    load_option(L"compile_options", from, &o_CompileOptions);

    load_option(L"link_options", from, &o_LinkOptions);

    load_option(L"link_l", from, &o_LinkLibs);

    if(o_ObjDir.value.characteres[o_ObjDir.value.length - 1] == L'\\' || o_ObjDir.value.characteres[o_ObjDir.value.length - 1] == L'/')
        strutf16_remove_index(&o_ObjDir.value, o_ObjDir.value.length - 1);

    if(load_option(key_AutoExec, from, &o_AutoExec)) {
        if(
            g_AutoExec =
                _wcsicmp(o_AutoExec.value.characteres, L"1")    == 0 ||
                _wcsicmp(o_AutoExec.value.characteres, L"true") == 0 ||
                _wcsicmp(o_AutoExec.value.characteres, L"yes")  == 0
        )
            load_option(key_ExecArgs, from, &o_ExecArgs);
    }else
        g_AutoExec = FALSE;

    return 0;
}

void unload_options() {
    Option *options[] = {
        &o_Language, &o_ObjDir, &o_BinDir, &o_Includes,
        &o_Libs, &o_CompileOptions, &o_LinkOptions, &o_LinkLibs, &o_ExecName,
        &o_AutoExec, &o_ExecArgs
    };

    unsigned int i;
    for(i = 0; i < (unsigned int) (sizeof(options) / sizeof(Option *)); i++)
        unload_option(options[i]);
}

char selectCompiler() {
    create_string_utf16(&compiler);
    string_utf16_lower(&o_Language.value);
    if(wcscmp(o_Language.value.characteres, L"c") == 0) {
        g_ModeLanguage = C_LANGUAGE;
        string_utf16_set_value(&compiler, L"gcc");
        return 1;
    }else if(wcscmp(o_Language.value.characteres, L"c++") == 0 || wcscmp(o_Language.value.characteres, L"cpp") == 0) {
        g_ModeLanguage = CPP_LANGUAGE;
        string_utf16_set_value(&compiler, L"g++");
        return 2;
    }
    return 0;
}

void free_all() {
    unload_options();
    free(programFilename.characteres);
    free(pwd.characteres);
    free(compiler.characteres);
}

int main(int argc, char **argv) {
    // On vérifie que le programme est exécuté via un terminal, et pas en double cliquant dessus
    HWND consoleWnd = GetConsoleWindow();
    DWORD processID;
    GetWindowThreadProcessId(consoleWnd, &processID);
    if(GetCurrentProcessId() == processID) {
        FreeConsole();
        MessageBoxW(NULL, L"Le programme doit être exécuté depuis un terminal.", L"Erreur !", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    set_console_UTF16();

    if(!check_args(argc, argv))
        return 0;

    String_UTF8  fileUtf8;
    String_UTF16 fileUtf16;
    create_string_utf16(&fileUtf16);
    if(!open_utf8_file(&fileUtf8, OPTIONS_FILENAME)) {
        fwprintf(stderr, L"Le fichier Cakefile n'existe pas dans ce dossier, fais `cake --generate` pour en créer un !\n");
        return 1;
    }

    string_utf8_to_utf16(&fileUtf8, &fileUtf16);
    free(fileUtf8.bytes);

    char result = load_options(&fileUtf16);
    free(fileUtf16.characteres);
    if(result != 0) {
        fwprintf(stderr, L"Une ou plusieurs clés obligatoires n'ont pas été trouvées.\n");
        return 1;
    }

    if(!selectCompiler()) {
        fwprintf(stderr, L"Le langage \"%s\" n'est pas pris en charge...\n", o_Language.value.characteres);
        unload_options();
        return 1;
    }

    create_string_utf16(&programFilename);
    create_string_utf16(&pwd);
    get_program_file_name(&programFilename);
    get_current_process_location(&pwd);
    
    String_UTF16 exec;
    string_utf16_copy(&o_BinDir.value, &exec);
    if(o_BinDir.value.characteres[o_BinDir.value.length - 1] != L'\\' && o_BinDir.value.characteres[o_BinDir.value.length - 1] != L'/')
        string_utf16_add_char(&exec, L'\\');
    string_utf16_add(&exec, o_ExecName.value.characteres);

    char commandResult;

    // On récupère la liste de tous les fichiers C du dossier et des sous dossiers.
    String_UTF16 out_dirC;
    String_UTF16 dirCommand;
    create_string_utf16(&dirCommand);

    // Je fais chcp 65001>NUL plusieurs fois car y a un bug et des fois la commande s'exécute pas
    string_utf16_set_value(&dirCommand, L"cmd /c chcp 65001>NUL & chcp 65001>NUL & chcp 65001>NUL & dir /b/s *.c");

    if(wcscmp(compiler.characteres, L"g++") == 0)
        string_utf16_add(&dirCommand, L" *.cpp *.c++");

    if((commandResult = execute_command(dirCommand.characteres, &out_dirC, NULL, NULL, 0, 0)) != 0)
        fwprintf(stderr, L"Erreur lors de l'exécution de la commande : %d\n", commandResult);
    
    //wprintf(L"Résultat :\n%s\n", out_dirC.characteres);

    free(dirCommand.characteres);

    if(out_dirC.length == 0) {
        fwprintf(stderr, L"Aucun fichier trouvé...\n");
        free(out_dirC.characteres);
        free_all();
        return 1;
    }

    // Liste des fichiers sources, pour compiler.
    String_UTF16 **listC = NULL;
    unsigned long listCsize = 0;

    // Liste des fichiers O, pour linker.
    String_UTF16 **listO = NULL;
    unsigned long listOsize = 0;

    listOsize = list_o_files(&listO, &out_dirC);
    unsigned long i;

    /*
    for(i = 0; i < listOsize; i++)
        wprintf(L"%s\n", listO[i]->characteres);
    */

    unsigned long *list = NULL;
    unsigned long needCompileNumber = 0;
    

    unsigned long long startTime;
    unsigned long compileNumber = 0;

    char errorCompile = -1;
    char errorLink = -1;

    switch(g_Mode) {
        case MODE_CLEAN:{
            clean: ;
            wprintf(L"Nettoyage...\n");
            for(i = 0; i < listOsize; i++) {
                wprintf(L"Suppression de \"%s\"...\n", listO[i]->characteres);
                _wremove(listO[i]->characteres);
            }
            wprintf(L"Suppression de \"%s\"...\n", exec.characteres);
            _wremove(exec.characteres);
            // C'est pas safe du tout, ça supprime tout sans vérifier ce que c'est :
            /*
            String_UTF16 cleanCommand;
            create_string_utf16(&cleanCommand);
            string_utf16_set_value(&cleanCommand, L"cmd /c if exist ");
            string_utf16_add(&cleanCommand, o_objDir.value.characteres);
            string_utf16_add(&cleanCommand, L" rd /q/s ");
            string_utf16_add(&cleanCommand, o_objDir.value.characteres);
            
            if((result = execute_command(cleanCommand.characteres, NULL, NULL, NULL)) != 0) {
                error_execute_command(cleanCommand.characteres, result);
                free(cleanCommand.characteres);
                free(out_dirC.characteres);
                free(exec.characteres);
                free_all();
                return 1;
            }
            free(cleanCommand.characteres);
            */
            if(g_Mode == MODE_RESET) goto reset;
            free(out_dirC.characteres);
            free(exec.characteres);
            free_all();
            return 1;
        }
        case MODE_RESET:
            goto clean;
            reset:
        case MODE_ALL:{
            listCsize = list_files(&listC, &out_dirC);
            free(out_dirC.characteres);
            mkdirs(o_ObjDir.value.characteres);

            needCompileNumber = check_who_must_compile(&list, listO, listC, listOsize);

            for(i = 0; i < needCompileNumber; i++) {
                String_UTF16 copy;
                string_utf16_copy(listO[list[i]], &copy);
                string_utf16_remove_part_from_end(&copy, L'\\');
                mkdirs(copy.characteres);
                free(copy.characteres);
            }
            startTime = get_current_time_millis();
            if(needCompileNumber) {
                wprintf(L"==========[ Compilation ]==========\n");
                for(i = 0; i < needCompileNumber; i++) {
                    if(create_object(listC[list[i]], listO[list[i]], &errorCompile) == 0 && errorCompile == 0)
                        compileNumber++;
                }
                wprintf(L"===================================\n\n\n");
            }
        }
        case MODE_LINK:
            if(g_Mode == MODE_LINK)
                startTime = get_current_time_millis();
            mkdirs(o_BinDir.value.characteres);
            if(g_Mode == MODE_LINK || (compileNumber > 0 && compileNumber == needCompileNumber)) {
                wprintf(L"==========[ Link ]==========\n");
                
                String_UTF16 linkCommand;

                // gcc
                string_utf16_copy(&compiler, &linkCommand);

                // gcc --option
                if(o_LinkOptions.loaded) {
                    string_utf16_add_char(&linkCommand, L' ');
                    string_utf16_add(&linkCommand, o_LinkOptions.value.characteres);
                }

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" 
                for(i = 0; i < listOsize; i++) {
                    string_utf16_add(&linkCommand, L" \"");
                    string_utf16_add(&linkCommand, listO[i]->characteres);
                    string_utf16_add_char(&linkCommand, L'\"');
                }

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" 
                string_utf16_add(&linkCommand, L" -o \"");
                string_utf16_add(&linkCommand, exec.characteres);
                string_utf16_add_char(&linkCommand, L'\"');

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" -I"C:\oui" 
                if(o_Includes.loaded) {
                    string_utf16_replace_all_char(&o_Includes.value, L'/', L'\\');
                    string_utf16_add(&linkCommand, o_Includes.value.characteres);
                }
                if(o_Libs.loaded) {
                    string_utf16_replace_all_char(&o_Libs.value, L'/', L'\\');
                    string_utf16_add(&linkCommand, o_Libs.value.characteres);
                }

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" -I"C:\oui" -L"C:\non" -lhello
                if(o_LinkLibs.loaded) {
                    string_utf16_add_char(&linkCommand, L' ');
                    string_utf16_add(&linkCommand, o_LinkLibs.value.characteres);
                }

                wprintf(L"%s\n", linkCommand.characteres);

                if((result = execute_command(linkCommand.characteres, NULL, NULL, &errorLink, 1, 1)) != 0)
                    error_execute_command(linkCommand.characteres, result);
                free(linkCommand.characteres);
                wprintf(L"============================\n\n\n");
            }
            break;
        default:
            break;
    }

    unsigned long long endTime = get_current_time_millis();
    wprintf(L"==========[ Stats ]==========\n");
    switch(g_Mode) {
        default:{
            wprintf(L"Aucune stat pour ce mode...\n");
            break;
        }
        case MODE_ALL:
        case MODE_RESET:{
            if (compileNumber > 0)
                wprintf(
                    L"Fichiers compilés : %lu / %lu (%.02f%%)\n"
                    L"Compilés%s en %llu ms.\n",
                    compileNumber, needCompileNumber, (float) ((float) compileNumber * 100.0f / (float) needCompileNumber), (errorLink == 0 ? L" et linkés" : L""), endTime - startTime
                );
            else
                wprintf(L"Rien n'a changé...\n");
            break;
        }
        case MODE_LINK:{
            if(errorLink == 0)
                wprintf(L"Linkés en %llu ms.\n", endTime - startTime);
            else
                wprintf(L"Rien n'a changé...\n");
            break;
        }
    }
    wprintf(L"=============================\n\n");

    // Exécution automatique
    if(g_AutoExec && compileNumber == needCompileNumber && (errorLink == -1 || errorLink == 0)) {
        String_UTF16 autoCommand;
        create_string_utf16(&autoCommand);
        string_utf16_set_value(&autoCommand, L".\\");
        string_utf16_add(&autoCommand, exec.characteres);
        if(o_ExecArgs.value.characteres != NULL) {
            string_utf16_add_char(&autoCommand, L' ');
            string_utf16_add(&autoCommand, o_ExecArgs.value.characteres);
        }
        wprintf(L"%s\n", autoCommand.characteres);
        startTime = get_current_time_millis();
        if(execute_command(autoCommand.characteres, NULL, NULL, NULL, 1, 1) != 0)
            wprintf(L"[%S] Erreur lors de l'exécution du programme (%lu)\n", PROGRAM_NAME, GetLastError());
        else
            wprintf(L"[%S] Durée d'exécution du programme : %llu ms.\n", PROGRAM_NAME, get_current_time_millis() - startTime);
        free(autoCommand.characteres);
    }

    free(list);

    if(listC) {
        for(i = 0; i < listCsize; i++)
            free(listC[i]->characteres);
        free(listC);
    }

    if(listO) {
        for(i = 0; i < listOsize; i++)
            free(listO[i]->characteres);
        free(listO);
    }

    free_all();

    return 0;
}
