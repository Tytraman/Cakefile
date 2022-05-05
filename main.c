#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <libcake/file.h>
#include <libcake/win.h>
#include <libcake/process.h>
#include <libcake/time.h>
#include <libcake/console.h>

#include "global.h"
#include "cakefile.h"


#ifdef CAKE_WINDOWS
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    #ifdef CAKE_WINDOWS
    // On vérifie que le programme est exécuté via un terminal, et pas en double cliquant dessus
    if(cake_is_double_clicked()) {
        MessageBoxW(NULL, L"Le programme doit être lancé depuis un terminal.", L"Erreur", MB_OK);
        return 1;
    }
    UINT consoleOutputCP = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);
    #endif

    // Récupère le mode passé.
    if(!check_args(argc, argv))
        return 0;

    // Chargement des options :
    Cake_FileObject *config = cake_fileobject_load(OPTIONS_FILENAME);
    if(config == NULL) {
        fprintf(stderr, "Fichier `%s` inexistant.\n", OPTIONS_FILENAME);
        return 1;
    }
    if(!get_fileobject_elements(config)) {
        cake_free_fileobject(config);
        return 1;
    }
    
    if(g_Mode & MODE_CLEAN_ENABLED) {
        if(!g_Quiet)
            printf("[===== Nettoyage =====]\n");
        Cake_List_String_UTF8 *files = cake_list_strutf8();
        Cake_List_String_UTF8 *folders = cake_list_strutf8();
        cake_list_files_recursive((cchar_ptr) o_ObjDir->value->bytes, files, folders, list_o_files_callback, NULL);
        ulonglong i;
        for(i = 0; i < files->data.length; ++i) {
            if(!g_Quiet)
                printf("Suppression du fichier \"%s\"...\n", files->list[i]->bytes);
            cake_delete_file((cchar_ptr) files->list[i]->bytes);
        }
        for(i = 0; i < folders->data.length; ++i) {
            if(!g_Quiet)
                printf("Suppresion du dossier \"%s\"...\n", folders->list[folders->data.length - i - 1]->bytes); 
            cake_delete_folder((cchar_ptr) folders->list[folders->data.length - i - 1]->bytes);
        }
        if(!g_Quiet)
            printf("Suppression du dossier \"%s\"...\n", o_ObjDir->value->bytes);
        cake_delete_folder((cchar_ptr) o_ObjDir->value->bytes);

        Cake_String_UTF8 *finalExec = cake_strutf8("");
        cake_strutf8_copy(finalExec, o_BinDir->value);
        cake_strutf8_add_char_array(finalExec, (cchar_ptr) o_ExecName->value->bytes);
        if(!g_Quiet)
            printf("Suppresion du fichier \"%s\"...\n", finalExec->bytes);
        cake_delete_file((cchar_ptr) finalExec->bytes);

        cake_free_strutf8(finalExec);
        cake_free_list_strutf8(folders);
        cake_free_list_strutf8(files);

        if(g_Mode == MODE_CLEAN_ENABLED)
            goto end;
    }
    srand(time(NULL));
    ulonglong compileTimeStart = 0, compileTimeEnd = 0;
    ulonglong linkTimeStart = 0, linkTimeEnd = 0;
    ulonglong i;

    Cake_List_String_UTF8 *srcFiles = NULL;
    Cake_List_String_UTF8 *oFiles = NULL;

    // Compilation des fichiers sources en objets
    if(g_Mode & MODE_COMPILE_ENABLED) {
        Cake_List_String_UTF8 *compileCommand = command_format(o_CompileCommandFormat->value);
        srcFiles = cake_list_strutf8();

        // Récupération des fichiers sources du projet
        cake_list_files_recursive(".", srcFiles, NULL, list_files_callback, o_SrcExtensions);

        // Listing des fichiers objets
        ulonglong j;
        oFiles = cake_list_strutf8();
        for(i = 0; i < srcFiles->data.length; ++i) {
            cake_list_strutf8_add_char_array(oFiles, (cchar_ptr) srcFiles->list[i]->bytes);
            j = 0;
            while(
                oFiles->list[i]->bytes[j] == '.' ||
                oFiles->list[i]->bytes[j] == FILE_SEPARATOR_CHAR ||
                oFiles->list[i]->bytes[j] == FILE_SEPARATOR_REVERSE_CHAR
            )
                j++;
            cake_strutf8_remove_from_to_internal(oFiles->list[i], 0, j);
            cake_strutf8_insert_char_array(oFiles->list[i], 0, (cchar_ptr) o_ObjDir->value->bytes);
            for(j = 0; j < o_SrcExtensions->data.length; ++j) {
                if(cake_strutf8_replace_end(oFiles->list[i], (cchar_ptr) o_SrcExtensions->list[j]->bytes, ".o"))
                    goto skip_obj;
            }
            cake_strutf8_add_char_array(oFiles->list[i], ".o");
        skip_obj: ;
        }

        uchar temp;
        uchar *ptr;
        ulonglong tempInternalIndex;
        if(g_Mode & MODE_REBUILD_ENABLED) {
            g_NeedCompileNumber = oFiles->data.length;
            goto compile;
        }

        // On vérifie si les fichiers sources doivent être recompilés
        cake_fd srcFd;
        cake_fd oFd;
        for(i = 0; i < oFiles->data.length; ++i) {
            // Si le fichier doit être compilé
            srcFd = cake_fdio_open_file((cchar_ptr) srcFiles->list[i]->bytes, CAKE_FDIO_ACCESS_READ, CAKE_FDIO_SHARE_READ, CAKE_FDIO_OPEN_IF_EXISTS, CAKE_FDIO_ATTRIBUTE_NORMAL);
            if(srcFd == CAKE_FDIO_ERROR_OPEN) {
                srcFiles->list[i]->bytes[0] = 3;
                continue;
            }
            oFd = cake_fdio_open_file((cchar_ptr) oFiles->list[i]->bytes, CAKE_FDIO_ACCESS_READ, CAKE_FDIO_SHARE_READ, CAKE_FDIO_OPEN_IF_EXISTS, CAKE_FDIO_ATTRIBUTE_NORMAL);
            if(oFd != CAKE_FDIO_ERROR_OPEN) {
                if(check_includes(srcFd, oFd, srcFiles->list[i]))
                    goto skip_time_check;

                if(cake_fdio_compare_time(srcFd, oFd, CAKE_FDIO_COMPARE_LAST_WRITE_TIME) != CAKE_FDIO_NEWER) {
                    srcFiles->list[i]->bytes[0] = 3;
                    cake_fdio_close(srcFd);
                    cake_fdio_close(oFd);
                    continue;
                }
            }
        skip_time_check:

            cake_fdio_close(srcFd);
            cake_fdio_close(oFd);
            g_NeedCompileNumber++;
        }

        if(g_NeedCompileNumber > 0) {
        compile:
            // On récupère l'index des noms de fichiers sources dans la commande
            Cake_UlonglongArray srcArray;
            command_index(&srcArray, compileCommand, "{src_file}");

            // On récupère l'index des noms de fichiers objets dans la commande
            Cake_UlonglongArray objArray;
            command_index(&objArray, compileCommand, "{obj_file}");

            if(!g_Quiet)
                printf("[===== Compilation =====]\n");
            compileTimeStart = cake_get_current_time_millis();

            // Compilation des fichiers un à un
            for(i = 0; i < srcFiles->data.length; ++i) {
                // On ignore les fichiers qui ne doivent pas être compilés
                if(srcFiles->list[i]->bytes[0] == 3)
                    continue;
                tempInternalIndex = oFiles->list[i]->data.length - 1;
                ptr = cake_strutf8_search_from_end(oFiles->list[i], FILE_SEPARATOR_CHAR_STR, &tempInternalIndex);
                // On crée les dossiers nécessaires
                if(ptr != NULL) {
                    temp = *ptr;
                    *ptr = '\0';
                    cake_mkdirs((cchar_ptr) oFiles->list[i]->bytes);
                    *ptr = temp;
                }

                // Remplacement de {src_file} par le fichier source à compiler
                command_replace_index(compileCommand, &srcArray, srcFiles, i);

                // Remplacement de {obj_file} par le fichier objet à obtenir
                command_replace_index(compileCommand, &objArray, oFiles, i);

                if(!g_Quiet) {
                    for(j = 0; j < compileCommand->data.length; ++j)
                        printf("%s ", compileCommand->list[j]->bytes);
                    printf("\n");
                }
                Cake_Process process;
                // Lancement de la commande pour compiler
                if(!cake_create_process(compileCommand, &process, NULL, NULL, NULL)) {
                    // Si une erreur est survenue au moment de la création du process
                    fprintf(stderr, "Une erreur est survenue au à la création du processus...\n");
                }else {
                    cake_process_start(process);
                    cake_exit_code retCode;
                    cake_process_wait(process, retCode);
                    if(retCode == 0)
                        g_CompileNumber++;
                }
            }
            compileTimeEnd = cake_get_current_time_millis();
            free(srcArray.array);
            free(objArray.array);
        }
        
        cake_free_list_strutf8(compileCommand);

        if((g_Mode & MODE_LINES_COUNT_ENABLED) == 0)
            cake_free_list_strutf8(srcFiles);
        if((g_Mode & MODE_LINK_ENABLED) == 0)
            cake_free_list_strutf8(oFiles);
    }

    uchar linkOk = 2;
    // Link des fichiers objets
    if(g_Mode & MODE_LINK_ENABLED) {
        // ./cake link
        if((g_Mode & MODE_COMPILE_ENABLED) == 0) {
            oFiles = cake_list_strutf8();
            cake_list_files_recursive((cchar_ptr) o_ObjDir->value->bytes, oFiles, NULL, list_o_files_callback, NULL);
            goto go_link;
        }
        if(g_NeedCompileNumber == 0 || g_CompileNumber != g_NeedCompileNumber)
            goto skip_link;

    go_link:
        cake_mkdirs((cchar_ptr) o_BinDir->value->bytes);
        Cake_List_String_UTF8 *linkCommand = command_format(o_LinkCommandFormat->value);
        command_replace_list(linkCommand, oFiles, "{list_obj_files}");
        
        if(!g_Quiet) {
            printf("[===== Link =====]\n");
            ulonglong y;
            for(y = 0; y < linkCommand->data.length; ++y)
                printf("%s ", linkCommand->list[y]->bytes);
            printf("\n");
        }
        Cake_Process process;
        if(!cake_create_process(linkCommand, &process, NULL, NULL, NULL))
            fprintf(stderr, "Une erreur est survenue à la création du processus...\n");
        else {
            linkTimeStart = cake_get_current_time_millis();
            cake_process_start(process);
            cake_exit_code retCode;
            cake_process_wait(process, retCode);
            linkTimeEnd = cake_get_current_time_millis();
            linkOk = (retCode == 0);
        }
        cake_free_list_strutf8(linkCommand);
        cake_free_list_strutf8(oFiles);
    }
skip_link:

    cake_exit_code retCode;
    ulonglong execStartTime = 0;
    ulonglong execEndTime   = 0;
    if(g_Mode & MODE_EXEC_ENABLED) {
        Cake_List_String_UTF8 *execCommand = cake_list_strutf8();
        cake_list_strutf8_add_char_array(execCommand, (cchar_ptr) o_BinDir->value->bytes);
        cake_strutf8_insert_char_array(execCommand->list[0], 0, "." FILE_SEPARATOR_CHAR_STR);
        if(execCommand->list[0]->bytes[execCommand->list[0]->data.length - 1] != FILE_SEPARATOR_CHAR)
            cake_strutf8_add_char_array(execCommand->list[0], FILE_SEPARATOR_CHAR_STR);
        cake_strutf8_add_char_array(execCommand->list[0], (cchar_ptr) o_ExecName->value->bytes);
        if(o_ExecArgs != NULL && o_ExecArgs->value->length > 0) {
            uchar *lastPtr = o_ExecArgs->value->bytes;
            uchar *ptr = lastPtr;
            cake_bool loop = cake_true;
            while(loop) {
                if(ptr == &o_ExecArgs->value->bytes[o_ExecArgs->value->data.length]) {
                    loop = cake_false;
                    goto options_spaced;
                }
                if(*ptr == ' ') {
                    *ptr = '\0';
                options_spaced:
                    cake_list_strutf8_add_char_array(execCommand, (cchar_ptr) lastPtr);
                    lastPtr = ptr + 1;
                }
                ptr++;
            }
        }
        if(!g_Quiet) {
            printf("[===== Exécution =====]\n");
            ulonglong y;
            for(y = 0; y < execCommand->data.length; ++y)
                printf("%s ", execCommand->list[y]->bytes);
            printf("\n");
        }

        Cake_Process process;
        execStartTime = cake_get_current_time_millis();
        cake_create_process(execCommand, &process, NULL, NULL, NULL);
        cake_process_start(process);
        cake_process_wait(process, retCode);
        execEndTime = cake_get_current_time_millis();
        cake_free_list_strutf8(execCommand);
    }

    if(g_Mode & MODE_STATS_ENABLED && !g_Quiet) {
        Cake_String_UTF8 *coolStat = cake_strutf8("[===== Stats =====]\n");
        if(g_Mode & MODE_COMPILE_ENABLED) {
            ulonglong t = compileTimeEnd - compileTimeStart;
            char *secBuffer    = cake_ulonglong_to_char_array_dyn(t / 1000);
            char *millisBuffer = cake_ulonglong_to_char_array_dyn(t % 1000);
            char *needCompilerNumberBuffer = cake_ulonglong_to_char_array_dyn(g_NeedCompileNumber);
            char *compileNumberBuffer      = cake_ulonglong_to_char_array_dyn(g_CompileNumber);
            cake_strutf8_add_char_array(coolStat, "Compilation: ");
            cake_strutf8_add_char_array(coolStat, compileNumberBuffer);
            cake_strutf8_add_char_array(coolStat, "/");
            cake_strutf8_add_char_array(coolStat, needCompilerNumberBuffer);
            cake_strutf8_add_char_array(coolStat, " [");
            cake_strutf8_add_char_array(coolStat, secBuffer);
            cake_strutf8_add_char_array(coolStat, ".");
            cake_strutf8_add_char_array(coolStat, millisBuffer);
            cake_strutf8_add_char_array(coolStat, "s]\n");
            free(secBuffer);
            free(millisBuffer);
            free(needCompilerNumberBuffer);
            free(compileNumberBuffer);
        }
        if(g_Mode & MODE_LINK_ENABLED) {
            ulonglong t = linkTimeEnd - linkTimeStart;
            char *secBuffer    = cake_ulonglong_to_char_array_dyn(t / 1000);
            char *millisBuffer = cake_ulonglong_to_char_array_dyn(t % 1000);
            cake_strutf8_add_char_array(coolStat, "Link: ");
            cake_strutf8_add_char_array(coolStat, (linkOk == 1 ? CONSOLE_FG_GREEN CONSOLE_BOLD "OK" CONSOLE_RESET " [" : (linkOk == 2 ? "Non effectué [" : CONSOLE_FG_RED CONSOLE_BOLD "Erreur" CONSOLE_RESET " [")));
            cake_strutf8_add_char_array(coolStat, secBuffer);
            cake_strutf8_add_char_array(coolStat, ".");
            cake_strutf8_add_char_array(coolStat, millisBuffer);
            cake_strutf8_add_char_array(coolStat, "s]\n");
            free(secBuffer);
            free(millisBuffer);
        }
        if(g_Mode & MODE_EXEC_ENABLED) {
            ulonglong t = execEndTime - execStartTime;
            char *secBuffer    = cake_ulonglong_to_char_array_dyn(t / 1000);
            char *millisBuffer = cake_ulonglong_to_char_array_dyn(t % 1000);
            #ifdef CAKE_UNIX
            char retBuffer[4];
            snprintf(retBuffer, sizeof(retBuffer), "%u", retCode);
            #else
            char retBuffer[11];
            snprintf(retBuffer, sizeof(retBuffer), 
            #ifdef CAKE_WINDOWS
            "%lu"
            #else
            "%u"
            #endif
            , retCode);
            #endif
            cake_strutf8_add_char_array(coolStat, "Code de retour: ");
            cake_strutf8_add_char_array(coolStat, retBuffer);
            cake_strutf8_add_char_array(coolStat, "\nTemps d'exécution: " CONSOLE_FG_YELLOW CONSOLE_BOLD);
            cake_strutf8_add_char_array(coolStat, secBuffer);
            cake_strutf8_add_char_array(coolStat, ".");
            cake_strutf8_add_char_array(coolStat, millisBuffer);
            cake_strutf8_add_char_array(coolStat, "s" CONSOLE_RESET "\n");
            free(secBuffer);
            free(millisBuffer);
        }
        printf((const char *) coolStat->bytes);
        cake_free_strutf8(coolStat);
    }

    if(g_Mode & MODE_LINES_COUNT_ENABLED) {
        if(srcFiles == NULL) {
            srcFiles = cake_list_strutf8();
            cake_list_files_recursive(".", srcFiles, NULL, list_files_callback, o_SrcExtensions);
        }
            
        ulonglong totalLines = 0;
        ulonglong currentLines;
        cake_fd fd;
        char c;
        if(!g_Quiet)
            printf("[===== Lignes =====]\n");
        for(i = 0; i < srcFiles->data.length; ++i) {
            currentLines = 0;
            fd = cake_fdio_open_file((cchar_ptr) srcFiles->list[i]->bytes, CAKE_FDIO_ACCESS_READ, CAKE_FDIO_SHARE_READ, CAKE_FDIO_OPEN_IF_EXISTS, CAKE_FDIO_ATTRIBUTE_NORMAL);
            if(fd != CAKE_FDIO_ERROR_OPEN) {
                #ifdef CAKE_UNIX
                while(read(fd, &c, sizeof(char))) {
                    if(c == '\n')
                        currentLines++;
                }
                #else
                // TODO: version Windows
                #endif
                cake_fdio_close(fd);
                totalLines += currentLines;
                printf("%s => %llu\n", (cchar_ptr) srcFiles->list[i]->bytes, currentLines);
            }
        }
        printf("Total: " CONSOLE_BOLD CONSOLE_FG_YELLOW "%llu" CONSOLE_RESET "\n", totalLines);
        cake_free_list_strutf8(srcFiles);
    }
    
end:
    cake_free_fileobject(config);
    //printf("Au revoir !\n");

    #ifdef CAKE_WINDOWS
    SetConsoleOutputCP(consoleOutputCP);
    #endif
    return 0;
}
