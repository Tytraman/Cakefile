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

// Vérifie les arguments passés au programme.
char check_args(int argc, char **argv);
unsigned long list_files(String_UTF16 ***listDest, String_UTF16 *src);
unsigned long list_o_files(String_UTF16 ***listDest, String_UTF16 *src);
unsigned long check_who_must_compile(unsigned long **list, String_UTF16 **listO, String_UTF16 **listC, unsigned long listOsize);
void check_includes(unsigned long **list, unsigned long *listSize, unsigned long current, String_UTF16 *fileC, String_UTF16 *fileO);
char create_object(String_UTF16 *cFile, String_UTF16 *oFile, char *error);

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
        }else if(strcmp(argv[1], "--version") == 0) {
            wprintf(L"%S x%S version %S\n", PROGRAM_NAME, (sizeof(void *) == 8 ? "64" : "86"), VERSION);
            return 0;
        }else if(strcasecmp(argv[1], "--generate") == 0) {
            char cakefile[] = "Cakefile";
            if(GetFileAttributesA(cakefile) == 0xffffffff) {
                unsigned char defaultCakefile[] =
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
                fwrite(defaultCakefile, 1, 131, pCakefile);
                fclose(pCakefile);
            }else
                wprintf(L"[%S] Il existe déjà un fichier `%S`.\n", PROGRAM_NAME, cakefile);
            return 0;
        }else {
            wprintf(L"[%S] Argument invalide, entre `cake --help` pour afficher l'aide.", PROGRAM_NAME);
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
    string_utf16_set_value(&u, objDir.characteres);
    string_utf16_add_char(&u, L'\\');
    for(i = 0; i < number; i++)
        if(!string_utf16_replace((*listDest)[i], srcDir.characteres, objDir.characteres))
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
    wchar_t space = L' ';
    
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
    string_utf16_add(&command, L"\" ");

    string_utf16_add(&command, compileOptions.characteres);
    string_utf16_add_char(&command, space);

    string_utf16_add(&command, includes.characteres);
    string_utf16_add_char(&command, space);

    string_utf16_add(&command, libs.characteres);

    wprintf(L"%s\n", command.characteres);

    char result;
    if((result = execute_command(command.characteres, NULL, NULL, error)) != 0)
        error_execute_command(command.characteres, result);
    free(command.characteres);
    return result;
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

    create_string_utf16(&programFilename);
    create_string_utf16(&pwd);
    get_program_file_name(&programFilename);
    get_current_process_location(&pwd);

    String_UTF8 fileUtf8;
    String_UTF16 fileUtf16;
    create_string_utf16(&fileUtf16);
    if(!open_utf8_file(&fileUtf8, L"Cakefile")) {
        error_file_not_found("Cakefile");
        return 1;
    }
    string_utf8_to_utf16(&fileUtf8, &fileUtf16);

    // Récupération des paramètres :
    create_string_utf16(&srcDir);
    create_string_utf16(&objDir);
    create_string_utf16(&binDir);
    create_string_utf16(&includes);
    create_string_utf16(&libs);
    create_string_utf16(&compileOptions);
    create_string_utf16(&linkOptions);
    create_string_utf16(&linkLibs);
    create_string_utf16(&exec);

    wchar_t *key1 = L"src_dir";
    wchar_t *key2 = L"obj_dir";
    wchar_t *key3 = L"bin_dir";
    wchar_t *key4 = L"exec_name";
    wchar_t *key5 = L"includes";
    wchar_t *key6 = L"libs";
    wchar_t *key7 = L"compile_options";
    wchar_t *key8 = L"link_options";
    wchar_t *key9 = L"link_l";

    if(!string_utf16_key_value(key1, &fileUtf16, &srcDir)) {
        error_key_not_found(key1);
        //goto program_end;
    }
    //wprintf(L"Clé 1 trouvée\n");

    if(!string_utf16_key_value(key2, &fileUtf16, &objDir)) {
        error_key_not_found(key2);
        //goto program_end;
    }
    //wprintf(L"Clé 2 trouvée\n");

    if(!string_utf16_key_value(key3, &fileUtf16, &binDir)) {
        error_key_not_found(key3);
        //goto program_end;
    }

    String_UTF16 execName;
    create_string_utf16(&execName);
    if(!string_utf16_key_value(key4, &fileUtf16, &execName))
        string_utf16_set_value(&execName, L"prog.exe");

    if(!string_utf16_key_value(key5, &fileUtf16, &includes))
        string_utf16_empty(&includes);

    if(!string_utf16_key_value(key6, &fileUtf16, &libs))
        string_utf16_empty(&libs);

    if(!string_utf16_key_value(key7, &fileUtf16, &compileOptions))
        string_utf16_empty(&compileOptions);

    if(!string_utf16_key_value(key8, &fileUtf16, &linkOptions))
        string_utf16_empty(&linkOptions);

    if(!string_utf16_key_value(key9, &fileUtf16, &linkLibs))
        string_utf16_empty(&linkLibs);

    char commandResult;

    // On récupère la liste de tous les fichiers C du dossier et des sous dossiers.
    String_UTF16 out_dirC;
    wchar_t *command_dirC = L"cmd /c chcp 65001>NUL & dir *.c /b/s";
    if((commandResult = execute_command(command_dirC, &out_dirC, NULL, NULL)) != 0) {
        wprintf(L"Erreur lors de l'exécution de la commande : %d\n", commandResult);
    }else {
        //wprintf(L"Résultat de la commande :\n%s\n", out_dirC.characteres);
    }

    // On stock le nom de l'exécutable avec son chemin relatif.
    string_utf16_set_value(&exec, binDir.characteres);
    string_utf16_add_char(&exec, L'\\');
    string_utf16_add(&exec, execName.characteres);
    string_utf16_replace_all_char(&exec, L'/', L'\\');
    free(execName.characteres);

    // Liste des fichiers C, pour compiler.
    String_UTF16 **listC = NULL;
    unsigned long listCsize = 0;

    // Liste des fichiers O, pour linker.
    String_UTF16 **listO = NULL;
    unsigned long listOsize = 0;

    if(out_dirC.characteres)
        listOsize = list_o_files(&listO, &out_dirC);

    char result;
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
            string_utf16_add(&cleanCommand, objDir.characteres);
            string_utf16_add(&cleanCommand, L" rd /q/s ");
            string_utf16_add(&cleanCommand, objDir.characteres);
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
            mkdirs(objDir.characteres);

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
            mkdirs(binDir.characteres);
            if(mode == MODE_LINK || (compileNumber > 0 && compileNumber == needCompileNumber)) {
                wprintf(L"==========Link==========\n");
                string_utf16_replace_all_char(&includes, L'/', L'\\');
                string_utf16_replace_all_char(&libs, L'/', L'\\');

                String_UTF16 linkCommand;
                create_string_utf16(&linkCommand);

                // gcc 
                string_utf16_set_value(&linkCommand, L"gcc ");

                // gcc --option 
                string_utf16_add(&linkCommand, linkOptions.characteres);
                string_utf16_add_char(&linkCommand, L' ');

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" 
                for(i = 0; i < listOsize; i++) {
                    string_utf16_add_char(&linkCommand, L'\"');
                    string_utf16_add(&linkCommand, listO[i]->characteres);
                    string_utf16_add(&linkCommand, L"\" ");
                }

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" 
                string_utf16_add(&linkCommand, L"-o \"");
                string_utf16_add(&linkCommand, exec.characteres);
                string_utf16_add_char(&linkCommand, L'\"');

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" -I"C:\oui" 
                if(includes.length) {
                    string_utf16_add(&linkCommand, L" -I\"");
                    string_utf16_add(&linkCommand, includes.characteres);
                    string_utf16_add_char(&linkCommand, L'\"');
                }

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" -I"C:\oui" -L"C:\non" 
                if(libs.length) {
                    string_utf16_add(&linkCommand, L" -L\"");
                    string_utf16_add(&linkCommand, libs.characteres);
                    string_utf16_add_char(&linkCommand, L'\"');
                }

                // gcc --option "fichier1.o" "fichier2.o" "fichier3.o" -o "prog.exe" -I"C:\oui" -L"C:\non" -lhello
                if(linkLibs.length) {
                    string_utf16_add_char(&linkCommand, L' ');
                    string_utf16_add(&linkCommand, linkLibs.characteres);
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
    free(list);
    free(out_dirC.characteres);
    free(programFilename.characteres);
    free(pwd.characteres);
    free(srcDir.characteres);
    free(objDir.characteres);
    free(binDir.characteres);
    free(includes.characteres);
    free(libs.characteres);
    free(compileOptions.characteres);
    free(linkOptions.characteres);
    free(linkLibs.characteres);
    free(exec.characteres);

    return 0;
}
