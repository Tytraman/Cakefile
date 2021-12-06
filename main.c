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
#include "include/funcs.h"

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
char exec_exec(String_UTF16 *exec, unsigned long long *duration);

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
        else if(strcasecmp(argv[1], "exec") == 0)
            g_Mode = MODE_EXEC;
        else if(strcasecmp(argv[1], "lines") == 0)
            g_Mode = MODE_LINES_COUNT;
        else if(strcasecmp(argv[1], "--help") == 0) {
            printf(
                "==========[ %s ]==========\n"
                "Lorsqu'aucun argument n'est passé, la commande est équivalente à `cake all`.\n"
                "[ Arguments ]\n"
                "> clean : supprime tous les fichiers objets et l'exécutable.\n"
                "> all : compile les fichiers modifiés puis crée l'exécutable.\n"
                "> reset : équivalent de `cake clean` puis `cake all`.\n"
                "> exec : exécute le programme avec les arguments dans l'option `exec_args`.\n"
                "> lines : affiche le nombre de lignes de chaque fichier puis le total.\n"
                "> --help : affiche ce message.\n"
                "> --version : affiche la version installée du programme.\n"
                "> --generate : génère un fichier `Cakefile` avec les options par défaut.\n\n"
                
                "Liste des options du fichier `Cakefile` :\n"
                "[ Obligatoires ]\n"
                "- language : langage de programmation utilisé.\n"
                "- obj_dir : dossier où sont stockés les fichiers `.o` une fois les fichiers `.c` compilés.\n"
                "- bin_dir : dossier où sera stocké l'exécutable final.\n"
                "- exec_name : nom de l'exécutable final.\n\n"

                "[ Optionnelles ]\n"
                "- includes : liste des dossiers includes externes à inclure.\n"
                "- libs : liste des dossiers de librairies externes à inclure.\n"
                "- compile_options : options utilisées pendant la compilation.\n"
                "- link_options : options utilisées pendant le link des fichiers objets.\n"
                "- link_l : librairies externes à inclure.\n"
                "- auto_exec : définie si le programme s'exécute automatiquement après sa génération. (true/false)\n"
                "- exec_args : liste des arguments à passer si auto_exec est activé ou si la commande `cake exec` a été tapée.\n\n"
                
                "Pour plus d'infos, voir la page github : https://github.com/Tytraman/Cakefile\n"
                "============================\n"
                , PROGRAM_NAME
            );
            return 0;
        }else if(strcasecmp(argv[1], "--version") == 0) {
            printf("%s x%s version %s\n", PROGRAM_NAME, (sizeof(void *) == 8 ? "64" : "86"), VERSION);
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
                fprintf(stderr, "[%s] Il existe déjà un fichier `%s`.\n", PROGRAM_NAME, OPTIONS_FILENAME);
            return 0;
        }else {
            fprintf(stderr, "[%s] Argument invalide, entre `cake --help` pour afficher l'aide.", PROGRAM_NAME);
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
            string_utf16_remove((*listDest)[size], g_Pwd.characteres);
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
    string_utf16_add_char(&insertObj, FILE_SEPARATOR);

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
        string_utf16_replace_all_char(listI[i], REVERSE_FILE_SEPARATOR, FILE_SEPARATOR);

        String_UTF16 fullPath;
        string_utf16_copy(fileC, &fullPath);
        if(!string_utf16_remove_part_from_end(&fullPath, FILE_SEPARATOR	))
            string_utf16_empty(&fullPath);
        else
            string_utf16_add_char(&fullPath, FILE_SEPARATOR);
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

    string_utf16_copy(&g_Compiler, &command);

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

    if(g_DrawProgressBar) {
        get_last_cursor_pos();
        clear_progress_bar();
    }

    String_UTF8 consoleCommand;
    strutf16_to_strutf8(&command, &consoleCommand);
    printf("%s\n", consoleCommand.bytes);
    free(consoleCommand.bytes);
    if(g_DrawProgressBar) {
        get_last_cursor_pos();
        draw_progress_bar(g_CurrentCompile + 1, g_NeedCompileNumber, g_ProgressBarWidthScale, g_ProgressBarFillChar, g_ProgressBarEmptyChar);
    }

    char result;
    if((result = execute_command(command.characteres, NULL, NULL, error, PRINT_STD)) != 0)
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
        string_utf16_replace_all_char(&temp, REVERSE_FILE_SEPARATOR, FILE_SEPARATOR);
        string_utf16_rtrim(&temp, FILE_SEPARATOR);
        end++;
        string_utf16_add(&o_Includes.value, L" -I \"");
        string_utf16_add(&o_Includes.value, temp.characteres);
        string_utf16_add_char(&o_Includes.value, L'\"');
        free(temp.characteres);
        begin = end;
    }

    string_utf16_copy_between(&copy, &temp, begin, copy.length);
    string_utf16_replace_all_char(&temp, REVERSE_FILE_SEPARATOR, FILE_SEPARATOR);
    string_utf16_rtrim(&temp, FILE_SEPARATOR);
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
        string_utf16_replace_all_char(&temp, REVERSE_FILE_SEPARATOR, FILE_SEPARATOR);
        string_utf16_rtrim(&temp, FILE_SEPARATOR);
        end++;
        string_utf16_add(&o_Libs.value, L" -L \"");
        string_utf16_add(&o_Libs.value, temp.characteres);
        string_utf16_add_char(&o_Libs.value, L'\"');
        free(temp.characteres);
        begin = end;
    }

    string_utf16_copy_between(&copy, &temp, begin, copy.length);
    string_utf16_replace_all_char(&temp, REVERSE_FILE_SEPARATOR, FILE_SEPARATOR);
    string_utf16_rtrim(&temp, FILE_SEPARATOR);
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

    if(o_ObjDir.value.characteres[o_ObjDir.value.length - 1] == FILE_SEPARATOR || o_ObjDir.value.characteres[o_ObjDir.value.length - 1] == REVERSE_FILE_SEPARATOR)
        strutf16_remove_index(&o_ObjDir.value, o_ObjDir.value.length - 1);

    if(load_option(key_AutoExec, from, &o_AutoExec)) {
        g_AutoExec =
                _wcsicmp(o_AutoExec.value.characteres, L"1")    == 0 ||
                _wcsicmp(o_AutoExec.value.characteres, L"true") == 0 ||
                _wcsicmp(o_AutoExec.value.characteres, L"yes")  == 0;
    }else
        g_AutoExec = FALSE;

    load_option(key_ExecArgs, from, &o_ExecArgs);

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
    create_string_utf16(&g_Compiler);
    string_utf16_lower(&o_Language.value);
    if(_wcsicmp(o_Language.value.characteres, L"c") == 0) {
        g_ModeLanguage = C_LANGUAGE;
        string_utf16_set_value(&g_Compiler, L"gcc");
        return 1;
    }else if(_wcsicmp(o_Language.value.characteres, L"c++") == 0 || _wcsicmp(o_Language.value.characteres, L"cpp") == 0) {
        g_ModeLanguage = CPP_LANGUAGE;
        string_utf16_set_value(&g_Compiler, L"g++");
        return 2;
    }
    return 0;
}

void free_all() {
    unload_options();
    free(g_ProgramFilename.characteres);
    free(g_Pwd.characteres);
    free(g_Compiler.characteres);
}

char exec_exec(String_UTF16 *exec, unsigned long long *duration) {
    *duration = 0;
    String_UTF16 command;
    create_string_utf16(&command);
    if(!strutf16_start_with(exec, L"." FILE_SEPARATOR_STR))
        string_utf16_set_value(&command, L"." FILE_SEPARATOR_STR);
    string_utf16_add(&command, exec->characteres);
    if(o_ExecArgs.value.characteres != NULL) {
        string_utf16_add_char(&command, L' ');
        string_utf16_add(&command, o_ExecArgs.value.characteres);
    }

    String_UTF8 consoleCommand;
    strutf16_to_strutf8(&command, &consoleCommand);
    printf("%s\n", consoleCommand.bytes);
    free(consoleCommand.bytes);
    unsigned long long startTime = get_current_time_millis();
    char result;
    String_UTF16 out, err;
    if((result = execute_command(command.characteres, NULL, NULL, NULL, PRINT_STD)) != 0) {
        printf("[%s] Erreur lors de l'exécution du programme (%d) (%lu)\n", PROGRAM_NAME, result, GetLastError());
        return 0;
    }
    *duration = get_current_time_millis() - startTime;
    free(command.characteres);
    return 1;   
}

int main(int argc, char **argv) {
    // On vérifie que le programme est exécuté via un terminal, et pas en double cliquant dessus
    HWND consoleWnd = GetConsoleWindow();
    DWORD processID;
    GetWindowThreadProcessId(consoleWnd, &processID);
    if(GetCurrentProcessId() == processID) {
        FreeConsole();
        MessageBoxW(NULL, L"Le programme doit être exécuté depuis un terminal.", L"Erreur !", MB_OK | MB_ICONERROR);
        return 1;
    }

    SetConsoleOutputCP(65001);

    g_Out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(g_Out, &g_ScreenInfo);

    CONSOLE_SCREEN_BUFFER_INFOEX infoex;
    infoex.cbSize = sizeof(infoex);
    GetConsoleScreenBufferInfoEx(g_Out, &infoex);
    COLORREF recoveryTable[16];
    memcpy(recoveryTable, infoex.ColorTable, sizeof(recoveryTable));
    infoex.srWindow.Bottom = g_ScreenInfo.srWindow.Bottom + 1;
    infoex.srWindow.Right  = g_ScreenInfo.srWindow.Right  + 1;
    infoex.ColorTable[0] = RGB(35, 0, 0);
    infoex.ColorTable[1] = RGB(45, 144, 224);
    infoex.ColorTable[2] = RGB(42, 250, 157);
    SetConsoleScreenBufferInfoEx(g_Out, &infoex);

    g_LastX = g_ScreenInfo.dwCursorPosition.X;
    g_LastY = g_ScreenInfo.dwCursorPosition.Y;

    if(!check_args(argc, argv))
        return 0;

    String_UTF8  fileUtf8;
    String_UTF16 fileUtf16;
    create_string_utf16(&fileUtf16);
    if(!open_utf8_file(&fileUtf8, OPTIONS_FILENAME)) {
        fprintf(stderr, "Le fichier Cakefile n'existe pas dans ce dossier, fais `cake --generate` pour en créer un !\n");
        return 1;
    }

    string_utf8_to_utf16(&fileUtf8, &fileUtf16);
    free(fileUtf8.bytes);

    char result = load_options(&fileUtf16);
    free(fileUtf16.characteres);
    if(result != 0) {
        fprintf(stderr, "Une ou plusieurs clés obligatoires n'ont pas été trouvées.\n");
        return 1;
    }

    if(!selectCompiler()) {
        String_UTF8 consoleCommandCompiler;
        strutf16_to_strutf8(&o_Language.value, &consoleCommandCompiler);
        fprintf(stderr, "Le langage \"%s\" n'est pas pris en charge...\n", consoleCommandCompiler.bytes);
        free(consoleCommandCompiler.bytes);
        unload_options();
        return 1;
    }

    create_string_utf16(&g_ProgramFilename);
    create_string_utf16(&g_Pwd);
    get_program_file_name(&g_ProgramFilename);
    get_current_process_location(&g_Pwd);
    
    String_UTF16 exec;
    string_utf16_copy(&o_BinDir.value, &exec);

    string_utf16_replace_all_char(&o_BinDir.value, REVERSE_FILE_SEPARATOR, FILE_SEPARATOR);
    if(exec.characteres[exec.length - 1] != FILE_SEPARATOR)
        string_utf16_add_char(&exec, FILE_SEPARATOR);
    string_utf16_add(&exec, o_ExecName.value.characteres);

    if(g_Mode == MODE_EXEC) {
        unsigned long long duration = 0;
        exec_exec(&exec, &duration);
        printf("[%s] Durée d'exécution du programme : %llu ms.\n", PROGRAM_NAME, duration);
        goto exec_end;
    }

    char commandResult;

    /* On récupère la liste de tous les fichiers C du dossier et des sous dossiers. */
    String_UTF16 out_dirC;
    String_UTF16 dirCommand;
    create_string_utf16(&dirCommand);

    // Je fais chcp 65001>NUL plusieurs fois car y a un bug et des fois la commande s'exécute pas
    string_utf16_set_value(&dirCommand, L"cmd /c chcp 65001>NUL & chcp 65001>NUL & chcp 65001>NUL & dir /b/s *.c");

    if(g_ModeLanguage == CPP_LANGUAGE)
        string_utf16_add(&dirCommand, L" *.cpp *.c++");

    if((commandResult = execute_command(dirCommand.characteres, &out_dirC, NULL, NULL, 0)) != 0)
        fprintf(stderr, "Erreur lors de l'exécution de la commande : %d\n", commandResult);

    free(dirCommand.characteres);

    /* ---------------------------------------------------------------------------- */

    if(out_dirC.length == 0) {
        fprintf(stderr, "Aucun fichier trouvé...\n");
        free(out_dirC.characteres);
        free_all();
        return 1;
    }

    String_UTF16 **listC = NULL;
    unsigned long listCsize = 0;

    String_UTF16 **listO = NULL;
    unsigned long listOsize = 0;

    listOsize = list_o_files(&listO, &out_dirC);
    unsigned long i;

    unsigned long *list = NULL;

    unsigned long long startTime;

    char errorCompile = -1;
    char errorLink = -1;

    switch(g_Mode) {
        case MODE_CLEAN:{
            clean: ;
            printf("Nettoyage...\n");
            String_UTF8 deletion;
            for(i = 0; i < listOsize; i++) {
                strutf16_to_strutf8(listO[i], &deletion);
                printf("Suppression de \"%s\"...\n", deletion.bytes);
                _wremove(listO[i]->characteres);
            }
            strutf16_to_strutf8(&exec, &deletion);
            printf("Suppression de \"%s\"...\n", deletion.bytes);
            free(deletion.bytes);
            _wremove(exec.characteres);
            if(g_Mode == MODE_RESET) goto reset;
            free(out_dirC.characteres);
            free(exec.characteres);
            free_all();
            return 1;
        }
        case MODE_RESET:{
            goto clean;
            reset: ;
        }
        case MODE_ALL:{
            listCsize = list_files(&listC, &out_dirC);
            free(out_dirC.characteres);
            mkdirs(o_ObjDir.value.characteres);

            g_NeedCompileNumber = check_who_must_compile(&list, listO, listC, listOsize);

            for(i = 0; i < g_NeedCompileNumber; i++) {
                String_UTF16 copy;
                string_utf16_copy(listO[list[i]], &copy);
                string_utf16_remove_part_from_end(&copy, FILE_SEPARATOR);
                mkdirs(copy.characteres);
                free(copy.characteres);
            }
            startTime = get_current_time_millis();
            if(g_NeedCompileNumber) {
                get_last_cursor_pos();
                printf("==========[ Compilation ]==========\n");
                get_last_cursor_pos();
                g_DrawProgressBar = 1;
                draw_progress_bar(g_CurrentCompile, g_NeedCompileNumber, g_ProgressBarWidthScale, g_ProgressBarFillChar, g_ProgressBarEmptyChar);
                for(g_CurrentCompile = 0; g_CurrentCompile < g_NeedCompileNumber; g_CurrentCompile++) {
                    if(create_object(listC[list[g_CurrentCompile]], listO[list[g_CurrentCompile]], &errorCompile) == 0 && errorCompile == 0)
                        g_CompileNumber++;
                    get_last_cursor_pos();
                    draw_progress_bar(g_CurrentCompile + 1, g_NeedCompileNumber, g_ProgressBarWidthScale, g_ProgressBarFillChar, g_ProgressBarEmptyChar);
                }
                g_DrawProgressBar = 0;
                get_last_cursor_pos();
                clear_progress_bar();
                printf("===================================\n\n\n");
            }
        }
        case MODE_LINK:{
            if(g_Mode == MODE_LINK) {
                free(out_dirC.characteres);
                startTime = get_current_time_millis();
            }
            mkdirs(o_BinDir.value.characteres);
            if(g_Mode == MODE_LINK || (g_CompileNumber > 0 && g_CompileNumber == g_NeedCompileNumber)) {
                printf("==========[ Link ]==========\n");
                
                String_UTF16 linkCommand;

                // gcc
                string_utf16_copy(&g_Compiler, &linkCommand);

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
                    string_utf16_replace_all_char(&o_Includes.value, REVERSE_FILE_SEPARATOR, FILE_SEPARATOR);
                    string_utf16_add(&linkCommand, o_Includes.value.characteres);
                }
                if(o_Libs.loaded) {
                    string_utf16_replace_all_char(&o_Libs.value, REVERSE_FILE_SEPARATOR, FILE_SEPARATOR);
                    string_utf16_add(&linkCommand, o_Libs.value.characteres);
                }

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" -I"C:\oui" -L"C:\non" -lhello
                if(o_LinkLibs.loaded) {
                    string_utf16_add_char(&linkCommand, L' ');
                    string_utf16_add(&linkCommand, o_LinkLibs.value.characteres);
                }
                
                String_UTF8 linkConsole;
                strutf16_to_strutf8(&linkCommand, &linkConsole);
                printf("%s\n", linkConsole.bytes);
                free(linkConsole.bytes);

                if((result = execute_command(linkCommand.characteres, NULL, NULL, &errorLink, PRINT_STD)) != 0)
                    error_execute_command(linkCommand.characteres, result);
                free(linkCommand.characteres);
                printf("============================\n\n\n");
            }
            break;
        }
        case MODE_LINES_COUNT:{
            printf("==========[ Lignes ]==========\n");
            /* On récupère la liste de tous les fichiers .h du dossier et des sous dossiers. */
            String_UTF16 out_DirH;
            String_UTF16 dirHCommand;
            create_string_utf16(&dirHCommand);

            // Je fais chcp 65001>NUL plusieurs fois car y a un bug et des fois la commande s'exécute pas
            string_utf16_set_value(&dirHCommand, L"cmd /c chcp 65001>NUL & chcp 65001>NUL & chcp 65001>NUL & dir /b/s *.h");

            if(g_ModeLanguage == CPP_LANGUAGE)
                string_utf16_add(&dirHCommand, L" *.hpp");

            if((commandResult = execute_command(dirHCommand.characteres, &out_DirH, NULL, NULL, 0)) != 0)
                fprintf(stderr, "Erreur lors de l'exécution de la commande : %d\n", commandResult);

            free(dirHCommand.characteres);

            /* ---------------------------------------------------------------------------- */
            String_UTF16 **listH = NULL;
            unsigned long listHsize = list_files(&listH, &out_DirH);
            listCsize = list_files(&listC, &out_dirC);
            unsigned long j;
            unsigned long statLines = 1;
            unsigned long long statTotalLines = 1;
            String_UTF8 sourceFile8;
            String_UTF16 sourceFile16;
            create_string_utf16(&sourceFile16);

            String_UTF16 ***targetList = &listH;
            unsigned long *listSize = &listHsize;
lines_count_loop: ;
            String_UTF8 target8;
            create_string_utf8(&target8);
            for(i = 0; i < *listSize; i++) {
                if(open_utf8_file(&sourceFile8, (*targetList)[i]->characteres)) {
                    string_utf8_to_utf16(&sourceFile8, &sourceFile16);
                    free(sourceFile8.bytes);
                    for(j = 0; j < sourceFile16.length; j++) {
                        if(sourceFile16.characteres[j] == L'\n')
                            statLines++;
                    }
                    statTotalLines += statLines;
                    strutf16_to_strutf8((*targetList)[i], &target8);
                    printf("%s : %lu\n", target8.bytes, statLines);
                    statLines = 1;
                }
            }
            free(target8.bytes);
            if(listSize == &listHsize) {
                targetList = &listC;
                listSize = &listCsize;
                goto lines_count_loop;
            }
            for(i = 0; i < listHsize; i++)
                free(listH[i]->characteres);
            free(listH);
            free(sourceFile16.characteres);
            printf(
                "\nTotal : %llu\n"
                "==============================\n"
                , statTotalLines
            );
            break;
        }
        default: break;
    }

    // Statistiques une fois les opérations terminées
    if(g_Mode & MODE_STATS_ENABLED) {
        unsigned long long endTime = get_current_time_millis();
        printf("==========[ Stats ]==========\n");
        switch(g_Mode) {
            default: break;
            case MODE_ALL:
            case MODE_RESET:{
                if(g_CompileNumber > 0)
                    printf(
                        "Fichiers compilés : %lu / %lu (%.02f%%)\n"
                        "Compilés%s en %llu ms.\n",
                        g_CompileNumber, g_NeedCompileNumber, (float) ((float) g_CompileNumber * 100.0f / (float) g_NeedCompileNumber), (errorLink == 0 ? " et linkés" : ""), endTime - startTime
                    );
                else
                    printf("Rien n'a changé...\n");
                break;
            }
            case MODE_LINK:{
                if(errorLink == 0)
                    printf("Linkés en %llu ms.\n", endTime - startTime);
                else
                    printf("Rien n'a changé...\n");
                break;
            }
        }
        printf("=============================\n\n");
    }

    // Exécution automatique
    if(g_AutoExec && g_CompileNumber == g_NeedCompileNumber && (errorLink == -1 || errorLink == 0)) {
        unsigned long long duration;
        exec_exec(&exec, &duration);
        printf("[%S] Durée d'exécution du programme : %llu ms.\n", PROGRAM_NAME, duration);
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

exec_end:
    free_all();

    for(i = 0; i < 16; i++)
        infoex.ColorTable[i] = recoveryTable[i];

    SetConsoleScreenBufferInfoEx(g_Out, &infoex);

    return 0;
}
