#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "include/libcake/file.h"
#include "include/libcake/win.h"
#include "include/libcake/process.h"

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
    Cake_FileObject *config = cake_file_object_load(OPTIONS_FILENAME);
    if(config == NULL) {
        fprintf(stderr, "Fichier `%s` inexistant.\n", OPTIONS_FILENAME);
        return 1;
    }

    if(!get_fileobject_elements(config)) {
        cake_free_file_object(config);
        return 1;
    }

    srand(time(NULL));
    ulonglong i;

    Cake_List_String_UTF8 *oFiles = NULL;
    Cake_String_UTF8 *includes = NULL;
    Cake_String_UTF8 *libs = NULL;
    if(
        (g_Mode & MODE_COMPILE_ENABLED) ||
        (g_Mode & MODE_LINK_ENABLED)
    ) {
        oFiles = cake_list_strutf8();
        if(o_Includes != NULL) {
            includes = cake_strutf8("");
            for(i = 0; i < o_Includes->elements.length; ++i) {
                cake_strutf8_add_char_array(includes, " -I\"");
                cake_strutf8_add_char_array(includes, o_Includes->elements.list[i]->key->bytes);
                cake_strutf8_add_char_array(includes, "\"");
            }
        }
        if(o_Libs != NULL) {
            libs = cake_strutf8("");
            for(i = 0; i < o_Libs->elements.length; ++i) {
                cake_strutf8_add_char_array(libs, " -L\"");
                cake_strutf8_add_char_array(libs, o_Libs->elements.list[i]->key->bytes);
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
                    if(o_CompileOptions != NULL && o_CompileOptions->value->length > 0) {
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
        
        // TODO: vérifier quels fichiers compiler

        uchar temp;
        ulonglong tempInternalIndex;
        uchar *ptr;
        Cake_String_UTF8 *finaleCompileCommand = cake_strutf8("");

        printf("[===== Compilation =====]\n");
        for(i = 0; i < oFiles->data.length; ++i) {
            tempInternalIndex = oFiles->list[i]->data.length - 1;
            ptr = cake_strutf8_search_from_end(oFiles->list[i], FILE_SEPARATOR_CHAR_STR, &tempInternalIndex);
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
            cake_process process;
            // Lancement de la commande pour compiler
            if(!cake_process_start(finaleCompileCommand->bytes, &process, NULL, NULL, NULL)) {
                // Si une erreur est survenue au moment du lancement du processus
                fprintf(stderr, "Une erreur est survenue au lancement du processus...\n");
            }else {
                cake_exit_code retCode;
                cake_process_wait(process, retCode);
            }
        }
        cake_free_strutf8(finaleCompileCommand);
        cake_free_strutf8(compileCommand);

        cake_free_list_strutf8(srcFiles);
        if(!(g_Mode & MODE_LINK_ENABLED))
            cake_free_list_strutf8(oFiles);
    }

    // Link des fichiers objets
    if(g_Mode & MODE_LINK_ENABLED) {
        if(oFiles == NULL) {
            oFiles = cake_list_strutf8();
            cake_list_files_recursive(".", oFiles, NULL, list_o_files_callback);
        }

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
                    if(o_LinkOptions != NULL && o_LinkOptions->value->length > 0) {
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
                    cake_process process;
                    if(!cake_process_start(linkCommand->bytes, &process, NULL, NULL, NULL))
                        fprintf(stderr, "Une erreur est survenue au lancement du processus...\n");
                    else {
                        cake_exit_code retCode;
                        cake_process_wait(process, retCode);
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

    if(includes != NULL)
        cake_free_strutf8(includes);
    if(libs != NULL)
        cake_free_strutf8(libs);

    cake_free_file_object(config);
    printf("Au revoir\n");

    #ifdef CAKE_WINDOWS
    SetConsoleOutputCP(consoleOutputCP);
    #endif
    return 0;
}
