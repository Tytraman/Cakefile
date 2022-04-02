#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "include/libcake/file.h"
#include "include/libcake/win.h"
#include "include/libcake/process.h"
#include "include/libcake/time.h"
#include "include/libcake/console.h"

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

    srand(time(NULL));
    ulonglong compileTimeStart = 0, compileTimeEnd = 0;
    ulonglong linkTimeStart = 0, linkTimeEnd = 0;
    ulonglong i;

    Cake_List_String_UTF8 *oFiles = NULL;
    Cake_String_UTF8 *includes = NULL;
    Cake_String_UTF8 *libs = NULL;
    if(
        (g_Mode & MODE_COMPILE_ENABLED) ||
        (g_Mode & MODE_LINK_ENABLED)
    ) {
        if(o_Includes != NULL) {
            includes = cake_strutf8("");
            for(i = 0; i < o_Includes->data.length; ++i) {
                cake_strutf8_add_char_array(includes, " -I\"");
                cake_strutf8_add_char_array(includes, o_Includes->list[i]->bytes);
                cake_strutf8_add_char_array(includes, "\"");
            }
        }
        if(o_Libs != NULL) {
            libs = cake_strutf8("");
            for(i = 0; i < o_Libs->data.length; ++i) {
                cake_strutf8_add_char_array(libs, " -L\"");
                cake_strutf8_add_char_array(libs, o_Libs->list[i]->bytes);
                cake_strutf8_add_char_array(libs, "\"");
            }
        }
    }

    // Compilation des fichiers sources en objets
    if(g_Mode & MODE_COMPILE_ENABLED) {
        Cake_List_String_UTF8 *srcExtensions = cake_list_strutf8();
        Cake_String_UTF8 *compileCommand = cake_strutf8(o_Compiler->value->bytes);
        switch(g_ModeLanguage) {
            case C_LANGUAGE:{
                cake_list_strutf8_add_char_array(srcExtensions, ".c");
                goto cpp_language;
            }
            case CPP_LANGUAGE:{
                cake_list_strutf8_add_char_array(srcExtensions, ".cpp");
                cake_list_strutf8_add_char_array(srcExtensions, ".c++");
                cake_list_strutf8_add_char_array(srcExtensions, ".cxx");
                cake_list_strutf8_add_char_array(srcExtensions, ".cc");
            cpp_language:
                cake_list_strutf8_add_char_array(srcExtensions, ".C");
                if(g_Mode & MODE_COMPILE_ENABLED) {
                    cake_strutf8_add_char_array(compileCommand, " \"{SRC}\" -c");

                    Cake_FileObjectElement *osCompileOptions = NULL;
                    #ifdef CAKE_WINDOWS
                    osCompileOptions = cake_fileobject_get_element(config, "windows.compile_options");
                    #else
                    osCompileOptions = cake_fileobject_get_element(config, "linux.compile_options");
                    #endif
                    if(osCompileOptions != NULL && osCompileOptions->value->length > 0) {
                        cake_strutf8_add_char_array(compileCommand, " ");
                        cake_strutf8_add_char_array(compileCommand, osCompileOptions->value->bytes);
                    }else if(o_CompileOptions != NULL && o_CompileOptions->value->length > 0) {
                        cake_strutf8_add_char_array(compileCommand, " ");
                        cake_strutf8_add_char_array(compileCommand, o_CompileOptions->value->bytes);
                    }
                    if(includes != NULL)
                        cake_strutf8_add_char_array(compileCommand, includes->bytes);
                    if(libs != NULL)
                        cake_strutf8_add_char_array(compileCommand, libs->bytes);
                    cake_strutf8_add_char_array(compileCommand, " -o \"{OBJ}\"");
                }
                break;
            }
        }
        Cake_List_String_UTF8 *srcFiles = cake_list_strutf8();

        // Récupération des fichiers sources du projet
        cake_list_files_recursive(".", srcFiles, NULL, list_files_callback);

        // Listing des fichiers objets
        uchar j;
        oFiles = cake_list_strutf8();
        for(i = 0; i < srcFiles->data.length; ++i) {
            cake_list_strutf8_add_char_array(oFiles, srcFiles->list[i]->bytes);
            cake_strutf8_insert_char_array(oFiles->list[i], 0, o_ObjDir->value->bytes);
            for(j = 0; j < srcExtensions->data.length; ++j) {
                if(cake_strutf8_replace_end(oFiles->list[i], srcExtensions->list[j]->bytes, ".o"))
                    goto skip_obj;
            }
            cake_strutf8_add_char_array(oFiles->list[i], ".o");
        skip_obj: ;
        }

        uchar temp;
        ulonglong tempInternalIndex;
        uchar *ptr;
        Cake_String_UTF8 *finaleCompileCommand = cake_strutf8("");
        cake_fd srcFd;
        cake_fd oFd;

        for(i = 0; i < oFiles->data.length; ++i) {
            // Si le fichier doit être compilé
            srcFd = cake_fdio_open_file(srcFiles->list[i]->bytes, CAKE_FDIO_ACCESS_READ, CAKE_FDIO_SHARE_READ, CAKE_FDIO_OPEN_IF_EXISTS, CAKE_FDIO_ATTRIBUTE_NORMAL);
            if(srcFd == CAKE_FDIO_ERROR_OPEN) {
                srcFiles->list[i]->bytes[0] = 3;
                continue;
            }
            oFd = cake_fdio_open_file(oFiles->list[i]->bytes, CAKE_FDIO_ACCESS_READ, CAKE_FDIO_SHARE_READ, CAKE_FDIO_OPEN_IF_EXISTS, CAKE_FDIO_ATTRIBUTE_NORMAL);
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
            printf("[===== Compilation =====]\n");
            compileTimeStart = cake_get_current_time_millis();
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
                    cake_mkdirs(oFiles->list[i]->bytes);
                    *ptr = temp;
                }
                cake_strutf8_copy(finaleCompileCommand, compileCommand);
                cake_strutf8_replace_all(finaleCompileCommand, "{SRC}", srcFiles->list[i]->bytes);
                cake_strutf8_replace_all(finaleCompileCommand, "{OBJ}", oFiles->list[i]->bytes);
                if(!g_Quiet)
                    printf("%s\n", finaleCompileCommand->bytes);
                Cake_Process process;
                // Lancement de la commande pour compiler
                if(!cake_create_process(finaleCompileCommand->bytes, &process, NULL, NULL, NULL)) {
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
        }
        
        cake_free_strutf8(finaleCompileCommand);
        cake_free_strutf8(compileCommand);

        cake_free_list_strutf8(srcFiles);
        if(!(g_Mode & MODE_LINK_ENABLED))
            cake_free_list_strutf8(oFiles);
    }

    uchar linkOk = 2;

    // Link des fichiers objets
    if(g_Mode & MODE_LINK_ENABLED) {
        // ./cake link
        if((g_Mode & MODE_COMPILE_ENABLED) == 0) {
            oFiles = cake_list_strutf8();
            cake_list_files_recursive(".", oFiles, NULL, list_o_files_callback);
            goto go_link;
        }

        // ./cake
        // ./cake all
        // ./cake reset
        if(
            (
                ((g_Mode & MODE_COMPILE_ENABLED) != 0) &&
                (
                    (g_NeedCompileNumber == 0)                ||
                    (g_CompileNumber != g_NeedCompileNumber)
                )
            )
        )
            goto skip_link;

    go_link:
        cake_mkdirs(o_BinDir->value->bytes);

        Cake_String_UTF8 *filenameLink = cake_strutf8(".cakefile_temp_link_");
        uchar linkDigit[6];
        uint digit;
        while(1) {
            digit = (uint) (rand() % 100000);
            cake_ulonglong_to_char_array(digit, linkDigit);
            cake_strutf8_add_char_array(filenameLink, linkDigit);
            if(!cake_file_exists(filenameLink->bytes))
                break;
            cake_strutf8_remove_from_to_internal(filenameLink, filenameLink->data.length - 5, filenameLink->data.length);
        }

        cake_fd linkFd = cake_fdio_open_file(filenameLink->bytes, CAKE_FDIO_ACCESS_WRITE, 0, CAKE_FDIO_OPEN_CREATE_ALWAYS, CAKE_FDIO_ATTRIBUTE_HIDDEN);
        if(linkFd == CAKE_FDIO_ERROR_OPEN) {
            fprintf(stderr, "Impossible de créer le fichier temporaire pour lier les fichiers objets.\n");
        }else {
            ulonglong i;
            Cake_String_UTF8 *linkCommand = cake_strutf8(o_Compiler->value->bytes);
            cake_strutf8_add_char_array(linkCommand, " @");
            cake_strutf8_add_char_array(linkCommand, filenameLink->bytes);
            const uchar space = ' ';
            cake_size bytesWritten;
            switch(g_ModeLanguage) {
                default:
                    cake_fdio_close(linkFd);
                    break;
                case C_LANGUAGE:
                case CPP_LANGUAGE:{
                    for(i = 0; i < oFiles->data.length; ++i) {
                        #ifdef CAKE_WINDOWS
                        cake_strutf8_replace_all(oFiles->list[i], FILE_SEPARATOR_CHAR_STR, FILE_SEPARATOR_REVERSE_CHAR_STR);
                        #endif
                        cake_fdio_write(linkFd, oFiles->list[i]->data.length, bytesWritten, oFiles->list[i]->bytes);
                        cake_fdio_write(linkFd, sizeof(space), bytesWritten, &space);
                    }
                    cake_fdio_close(linkFd);
                    Cake_FileObjectElement *osLinkOptions = NULL;
                    #ifdef CAKE_WINDOWS
                    osLinkOptions = cake_fileobject_get_element(config, "windows.link_options");
                    #else
                    osLinkOptions = cake_fileobject_get_element(config, "linux.link_options");
                    #endif
                    if(osLinkOptions != NULL && osLinkOptions->value->length > 0) {
                        cake_strutf8_add_char_array(linkCommand, " ");
                        cake_strutf8_add_char_array(linkCommand, osLinkOptions->value->bytes);
                    }else if(o_LinkOptions != NULL && o_LinkOptions->value->length > 0) {
                        cake_strutf8_add_char_array(linkCommand, " ");
                        cake_strutf8_add_char_array(linkCommand, o_LinkOptions->value->bytes);
                    }
                    if(o_LinkLibs != NULL && o_LinkLibs->value->length > 0) {
                        cake_strutf8_add_char_array(linkCommand, " ");
                        cake_strutf8_add_char_array(linkCommand, o_LinkLibs->value->bytes);
                    }
                    cake_strutf8_add_char_array(linkCommand, " -o ");
                    cake_strutf8_add_char_array(linkCommand, o_BinDir->value->bytes);
                    cake_strutf8_add_char_array(linkCommand, o_ExecName->value->bytes);
                    printf("[===== Link =====]\n");
                    if(!g_Quiet)
                        printf("%s\n", linkCommand->bytes);
                    Cake_Process process;
                    if(!cake_create_process(linkCommand->bytes, &process, NULL, NULL, NULL))
                        fprintf(stderr, "Une erreur est survenue à la création du processus...\n");
                    else {
                        linkTimeStart = cake_get_current_time_millis();
                        cake_process_start(process);
                        cake_exit_code retCode;
                        cake_process_wait(process, retCode);
                        linkTimeEnd = cake_get_current_time_millis();
                        linkOk = (retCode == 0);
                    }
                    break;
                }
            }
            cake_delete_file(filenameLink->bytes);
            cake_free_strutf8(linkCommand);
            cake_free_list_strutf8(oFiles);
        }
        cake_free_strutf8(filenameLink);
    }
skip_link:

    if(includes != NULL)
        cake_free_strutf8(includes);
    if(libs != NULL)
        cake_free_strutf8(libs);

    if(g_Mode & MODE_STATS_ENABLED) {
        Cake_String_UTF8 *coolStat = cake_strutf8("[===== Stats =====]\n");
        if(g_Mode & MODE_COMPILE_ENABLED) {
            ulonglong t = compileTimeEnd - compileTimeStart;
            uchar *secBuffer    = cake_ulonglong_to_char_array_dyn(t / 1000);
            uchar *millisBuffer = cake_ulonglong_to_char_array_dyn(t % 1000);
            uchar *needCompilerNumberBuffer = cake_ulonglong_to_char_array_dyn(g_NeedCompileNumber);
            uchar *compileNumberBuffer      = cake_ulonglong_to_char_array_dyn(g_CompileNumber);
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
            uchar *secBuffer    = cake_ulonglong_to_char_array_dyn(t / 1000);
            uchar *millisBuffer = cake_ulonglong_to_char_array_dyn(t % 1000);
            cake_strutf8_add_char_array(coolStat, "Link: ");
            cake_strutf8_add_char_array(coolStat, (linkOk == 1 ? CONSOLE_FG_GREEN CONSOLE_BOLD "OK" CONSOLE_RESET " [" : (linkOk == 2 ? "Non effectué [" : CONSOLE_FG_RED CONSOLE_BOLD "Erreur" CONSOLE_RESET " [")));
            cake_strutf8_add_char_array(coolStat, secBuffer);
            cake_strutf8_add_char_array(coolStat, ".");
            cake_strutf8_add_char_array(coolStat, millisBuffer);
            cake_strutf8_add_char_array(coolStat, "s]\n");
            free(secBuffer);
            free(millisBuffer);
        }
        printf(coolStat->bytes);
        cake_free_strutf8(coolStat);
    }
    cake_free_fileobject(config);

    #ifdef CAKE_WINDOWS
    SetConsoleOutputCP(consoleOutputCP);
    #endif
    return 0;
}
