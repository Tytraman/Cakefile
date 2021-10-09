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

//TODO: refaire le message de la commande help
//TODO: ajouter support g++

// Vérifie les arguments passés au programme.
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

char check_args(int argc, char **argv) {
    if(argc > 1) {
        if(strcasecmp(argv[1], "all") == 0)
            mode = MODE_ALL;
        else if(strcasecmp(argv[1], "reset") == 0)
            mode = MODE_RESET;
        else if(strcasecmp(argv[1], "link") == 0)
            mode = MODE_LINK;
        else if(strcasecmp(argv[1], "clean") == 0)
            mode = MODE_CLEAN;
        else if(strcasecmp(argv[1], "--help") == 0) {
            wprintf(
                L"==========%S==========\n"
                L"Lorsqu'aucun argument n'est passé, la commande est équivalente à `cake all`.\n\n"
                L"[ Arguments ]\n"
                L"> --help : affiche ce message.\n"
                L"> --version : affiche la version installée du programme.\n"
                L"> --generate : génère un fichier `Cakefile`.\n"
                L"> all : compile uniquement les fichiers sources modifiés ou dépendants d'headers modifiés, puis les link.\n"
                L"> reset : supprime tous les fichiers objets, l'exécutable et recompile tout, puis les link.\n"
                L"> clean : supprime tous les fichiers objets et l'exécutable.\n\n"
                L"Le compilateur utilisé est gcc, alors assure toi de l'avoir installé et d'avoir la variable d'environnement !\n"
                L"Les options suivantes doivent être mises dans un fichier `Cakefile` (sans extension).\n"
                L"Le format est clé : valeur\n\n"
                L"Liste des options :\n"
                L"[ Obligatoires ]\n"
                L"- src_dir : dossier où se situent les fichiers sources.\n"
                L"- obj_dir : dossier dans lequel les fichiers compilés logeront.\n"
                L"- bin_dir : dossier dans lequel le fichier exécutable prendra ses aises.\n\n"
                L"[ Optionnelles ]\n"
                L"- exec_name : nom du fichier exécutable final (par défaut : prog.exe)\n"
                L"- includes : liste des dossiers includes externes à inclure.\n"
                L"- libs : liste des dossiers de librairies externes à inclure.\n"
                L"- compile_options : options utilisées pendant la compilation.\n"
                L"- link_options : options utilisées pendant le link des fichiers objets.\n"
                L"- link_l : librairies externes à inclure.\n"
                L"========================\n"
                , PROGRAM_NAME
            );
            return 0;
        }else if(strcasecmp(argv[1], "--version") == 0) {
            wprintf(L"%S x%S version %S\n", PROGRAM_NAME, (sizeof(void *) == 8 ? "64" : "86"), VERSION);
            return 0;
        }else if(strcasecmp(argv[1], "--generate") == 0) {
            char cakefile[] = "Cakefile";
            if(GetFileAttributesA(cakefile) == 0xffffffff) {
                unsigned char defaultCakefile[] =
                    "language : c\n\n"
                    "src_dir : src\n"
                    "obj_dir : obj\n"
                    "bin_dir : bin\n\n"

                    "exec_name : prog.exe\n\n"

                    "includes : \n"
                    "libs : \n\n"

                    "compile_options : \n"
                    "link_options : \n\n"

                    "link_l : ";
                FILE *pCakefile = fopen(cakefile, "wb");
                fwrite(defaultCakefile, 1, 145, pCakefile);
                fclose(pCakefile);
            }else
                fwprintf(stderr, L"[%S] Il existe déjà un fichier `%S`.\n", PROGRAM_NAME, cakefile);
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
            *listDest = realloc(*listDest, (size + 1) * sizeof(String_UTF16 *));
            (*listDest)[size] = malloc(sizeof(String_UTF16));
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
    String_UTF16 srcCopy;
    create_string_utf16(&srcCopy);
    string_utf16_set_value(&srcCopy, src->characteres);

    unsigned long i;
    for(i = 0; i < srcCopy.length - 2; i++)
        if(srcCopy.characteres[i] == L'.' && (srcCopy.characteres[i + 1] == L'c' || srcCopy.characteres[i + 1] == L'C') && srcCopy.characteres[i + 2] == L'\r')
            srcCopy.characteres[i + 1] = L'o';

    unsigned long number = list_files(listDest, &srcCopy);
    free(srcCopy.characteres);

    String_UTF16 u;
    create_string_utf16(&u);
    string_utf16_set_value(&u, o_objDir.value.characteres);
    string_utf16_add_char(&u, L'\\');
    for(i = 0; i < number; i++)
        if(!string_utf16_replace((*listDest)[i], o_srcDir.value.characteres, o_objDir.value.characteres))
            string_utf16_insert((*listDest)[i], u.characteres);
    free(u.characteres);

    return number;
}

unsigned long check_who_must_compile(unsigned long **list, String_UTF16 **listO, String_UTF16 **listC, unsigned long listOsize) {
    unsigned long number = 0UL;
    unsigned long i;
    FILETIME lastModifiedO, lastModifiedC;
    HANDLE hFileO, hFileC;

    for(i = 0UL; i < listOsize; i++) {
        if(!file_exists(listO[i]->characteres)) {
            *list = realloc(*list, (number + 1) * sizeof(unsigned long));
            (*list)[number++] = i;
        }else {
            if((hFileO = CreateFileW(listO[i]->characteres, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
                if((hFileC = CreateFileW(listC[i]->characteres, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
                    GetFileTime(hFileO, NULL, NULL, &lastModifiedO);
                    GetFileTime(hFileC, NULL, NULL, &lastModifiedC);
                    if(CompareFileTime(&lastModifiedO, &lastModifiedC) == -1) {
                        CloseHandle(hFileO);
                        CloseHandle(hFileC);
                        *list = realloc(*list, (number + 1) * sizeof(unsigned long));
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

    wchar_t *search = L"#include";

    wchar_t *ptr;
    unsigned long index = 0;
    while((ptr = string_utf16_search_from(&fileUtf16, search, &index)) != NULL) {

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
        listI = realloc(listI, (listIsize + 1) * sizeof(String_UTF16 *));
        listI[listIsize] = malloc(sizeof(String_UTF16));
        listI[listIsize]->length = &fileUtf16.characteres[index] - ptr;
        listI[listIsize]->characteres = malloc(listI[listIsize]->length * sizeof(wchar_t) + sizeof(wchar_t));
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
                    *list = realloc(*list, (*listSize + 1) * sizeof(unsigned long));
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
    create_string_utf16(&command);

    // gcc -c "
    string_utf16_set_value(&command, L"gcc -c \"");

    // gcc -c "fichier.c
    string_utf16_add(&command, cFile->characteres);

    // gcc -c "fichier.c" -o "
    string_utf16_add(&command, L"\" -o \"");

    // gcc -c "fichier.c" -o "fichier.o
    string_utf16_add(&command, oFile->characteres);

    // gcc -c "fichier.c" -o "fichier.o"
    string_utf16_add_char(&command, L'\"');

    if(o_compileOptions.loaded) {
        string_utf16_add_char(&command, L' ');
        string_utf16_add(&command, o_compileOptions.value.characteres);
    }

    if(o_includes.loaded)
        string_utf16_add(&command, o_includes.value.characteres);

    if(o_libs.loaded)
        string_utf16_add(&command, o_libs.value.characteres);

    wprintf(L"%s\n", command.characteres);

    char result;
    if((result = execute_command(command.characteres, NULL, NULL, error)) != 0)
        error_execute_command(command.characteres, result);
    free(command.characteres);
    return result;
}

void combine_includes_path() {
    unsigned long begin = 0, end = 0;
                    
    String_UTF16 copy, temp;
    string_utf16_copy(&o_includes.value, &copy);
    string_utf16_empty(&o_includes.value);

    while(string_utf16_find(&copy, L'|', &end)) {
        string_utf16_copy_between(&copy, &temp, begin, end);
        string_utf16_rtrim(&temp, L'\\');
        end++;
        string_utf16_add(&o_includes.value, L" -I \"");
        string_utf16_add(&o_includes.value, temp.characteres);
        string_utf16_add_char(&o_includes.value, L'\"');
        free(temp.characteres);
        begin = end;
    }

    string_utf16_copy_between(&copy, &temp, begin, copy.length);
    string_utf16_rtrim(&temp, L'\\');
    string_utf16_add(&o_includes.value, L" -I \"");
    string_utf16_add(&o_includes.value, temp.characteres);
    string_utf16_add_char(&o_includes.value, L'\"');
    free(temp.characteres);
    free(copy.characteres);
}

void combine_libs_path() {
    unsigned long begin = 0, end = 0;
                    
    String_UTF16 copy, temp;
    string_utf16_copy(&o_libs.value, &copy);
    string_utf16_empty(&o_libs.value);

    while(string_utf16_find(&copy, L'|', &end)) {
        string_utf16_copy_between(&copy, &temp, begin, end);
        string_utf16_rtrim(&temp, L'\\');
        end++;
        string_utf16_add(&o_libs.value, L" -L \"");
        string_utf16_add(&o_libs.value, temp.characteres);
        string_utf16_add_char(&o_libs.value, L'\"');
        free(temp.characteres);
        begin = end;
    }

    string_utf16_copy_between(&copy, &temp, begin, copy.length);
    string_utf16_rtrim(&temp, L'\\');
    string_utf16_add(&o_libs.value, L" -L \"");
    string_utf16_add(&o_libs.value, temp.characteres);
    string_utf16_add_char(&o_libs.value, L'\"');
    free(temp.characteres);
    free(copy.characteres);
}

char load_options(String_UTF16 *from) {
    init_option(&o_srcDir);
    init_option(&o_objDir);
    init_option(&o_binDir);
    init_option(&o_includes);
    init_option(&o_libs);
    init_option(&o_compileOptions);
    init_option(&o_linkOptions);
    init_option(&o_linkLibs);
    init_option(&o_execName);

    wchar_t *key_language = L"language";
    wchar_t *key_srcDir   = L"src_dir";
    wchar_t *key_objDir   = L"obj_dir";
    wchar_t *key_binDir   = L"bin_dir";
    wchar_t *key_execName = L"exec_name";

    if(!load_option(key_language, from, &o_language)) {
        error_key_not_found(key_language);
        return 1;
    }

    if(!load_option(key_srcDir, from, &o_srcDir)) {
        error_key_not_found(key_srcDir);
        return 2;
    }

    if(!load_option(key_objDir, from, &o_objDir)) {
        error_key_not_found(key_objDir);
        return 3;
    }

    if(!load_option(key_binDir, from, &o_binDir)) {
        error_key_not_found(key_binDir);
        return 4;
    }

    if(!load_option(key_execName, from, &o_execName)) {
        error_key_not_found(key_execName);
        return 5;
    }

    if(load_option(L"includes", from, &o_includes))
        combine_includes_path();

    if(load_option(L"libs", from, &o_libs))
        combine_libs_path();

    load_option(L"compile_options", from, &o_compileOptions);

    load_option(L"link_options", from, &o_linkOptions);

    load_option(L"link_l", from, &o_linkLibs);

    return 0;
}

void unload_options() {
    Option *options[] = {
        &o_language, &o_srcDir, &o_objDir, &o_binDir, &o_includes,
        &o_libs, &o_compileOptions, &o_linkOptions, &o_linkLibs, &o_execName
    };

    unsigned int i;
    for(i = 0; i < 10; i++)
        unload_option(options[i]);
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
        error_file_not_found(OPTIONS_FILENAME);
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

    create_string_utf16(&programFilename);
    create_string_utf16(&pwd);
    get_program_file_name(&programFilename);
    get_current_process_location(&pwd);
    
    String_UTF16 exec;
    string_utf16_copy(&o_binDir.value, &exec);
    string_utf16_add_char(&exec, L'\\');
    string_utf16_add(&exec, o_execName.value.characteres);

    char commandResult;

    // On récupère la liste de tous les fichiers C du dossier et des sous dossiers.
    String_UTF16 out_dirC;
    wchar_t *command_dirC = L"cmd /c chcp 65001>NUL & dir *.c /b/s";
    if((commandResult = execute_command(command_dirC, &out_dirC, NULL, NULL)) != 0)
        fwprintf(stderr, L"Erreur lors de l'exécution de la commande : %d\n", commandResult);

    // Liste des fichiers C, pour compiler.
    String_UTF16 **listC = NULL;
    unsigned long listCsize = 0;

    // Liste des fichiers O, pour linker.
    String_UTF16 **listO = NULL;
    unsigned long listOsize = 0;

    if(out_dirC.characteres)
        listOsize = list_o_files(&listO, &out_dirC);

    unsigned long *list = NULL;
    unsigned long needCompileNumber;
    unsigned long i;

    unsigned long long startTime;
    unsigned long compileNumber = 0;

    char error, isLink = 0;

    switch(mode) {
        case MODE_CLEAN:{
            clean: ;
            String_UTF16 cleanCommand;
            create_string_utf16(&cleanCommand);
            string_utf16_set_value(&cleanCommand, L"cmd /c if exist ");
            string_utf16_add(&cleanCommand, o_objDir.value.characteres);
            string_utf16_add(&cleanCommand, L" rd /q/s ");
            string_utf16_add(&cleanCommand, o_objDir.value.characteres);
            wprintf(L"Nettoyage...\n");
            if((result = execute_command(cleanCommand.characteres, NULL, NULL, NULL)) != 0) {
                error_execute_command(cleanCommand.characteres, result);
                free(cleanCommand.characteres);
                goto end;
            }
            free(cleanCommand.characteres);
            _wremove(exec.characteres);
            if(mode == MODE_RESET) goto reset;
            goto end;
        }
        case MODE_RESET:
            goto clean;
            reset:
        case MODE_ALL:{
            if(out_dirC.characteres)
                listCsize = list_files(&listC, &out_dirC);
            mkdirs(o_objDir.value.characteres);

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
                wprintf(L"==========Compilation==========\n");
                for(i = 0; i < needCompileNumber; i++) {
                    if(create_object(listC[list[i]], listO[list[i]], &error) == 0 && error == 0)
                        compileNumber++;
                }
                wprintf(L"===============================\n\n\n");
            }
        }
        case MODE_LINK:
            if(mode == MODE_LINK)
                startTime = get_current_time_millis();
            mkdirs(o_binDir.value.characteres);
            if(mode == MODE_LINK || (compileNumber > 0 && compileNumber == needCompileNumber)) {
                wprintf(L"==========Link==========\n");
                

                String_UTF16 linkCommand;
                create_string_utf16(&linkCommand);

                // gcc
                string_utf16_set_value(&linkCommand, L"gcc");

                // gcc --option
                if(o_linkOptions.loaded) {
                    string_utf16_add_char(&linkCommand, L' ');
                    string_utf16_add(&linkCommand, o_linkOptions.value.characteres);
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
                if(o_includes.loaded) {
                    string_utf16_replace_all_char(&o_includes.value, L'/', L'\\');
                    string_utf16_add(&linkCommand, o_includes.value.characteres);
                }
                if(o_libs.loaded) {
                    string_utf16_replace_all_char(&o_libs.value, L'/', L'\\');
                    string_utf16_add(&linkCommand, o_libs.value.characteres);
                }

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" -I"C:\oui" -L"C:\non" -lhello
                if(o_linkLibs.loaded) {
                    string_utf16_add_char(&linkCommand, L' ');
                    string_utf16_add(&linkCommand, o_linkLibs.value.characteres);
                }

                wprintf(L"%s\n", linkCommand.characteres);

                if((result = execute_command(linkCommand.characteres, NULL, NULL, &error)) != 0)
                    error_execute_command(linkCommand.characteres, result);
                else if(error == 0)
                    isLink = 1;
                free(linkCommand.characteres);
                wprintf(L"========================\n\n\n");
            }
            break;
        default:
            break;
    }

    unsigned long long endTime = get_current_time_millis();
    wprintf(L"==========Stats==========\n");
    switch(mode) {
        default:
            wprintf(L"Aucune stat pour ce mode...\n");
            break;
        case MODE_ALL:
        case MODE_RESET:
            if (compileNumber > 0)
                wprintf(
                    L"Fichiers compilés : %lu / %lu\n"
                    L"Compilés%s en %llu ms.\n",
                    compileNumber, needCompileNumber, (isLink ? L" et linkés" : L""), endTime - startTime
                );
            else
                wprintf(L"Rien n'a changé...\n");
            break;
        case MODE_LINK:
            wprintf(L"Linkés en %llu ms.\n", endTime - startTime);
            break;
    }
    wprintf(L"=========================\n\n");

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

end:
    unload_options();
    free(out_dirC.characteres);
    free(programFilename.characteres);
    free(pwd.characteres);
    free(exec.characteres);

    return 0;
}
