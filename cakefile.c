#include "cakefile.h"

#include <stdio.h>
#include <stdlib.h>

#include "include/libcake/file.h"
#include "include/libcake/fdio.h"

#include "global.h"

#define WINDOWS_KEY "windows."
#define LINUX_KEY   "linux."

cake_bool check_args(int argc, cake_char *argv[]) {
    cake_bool ret = cake_true;
    if(argc > 1) {
        cake_bool help = cake_true;
        cake_bool version = cake_true;
        cake_bool invalid = cake_true;
        cake_bool generate = cake_true;
        int i;
        for(i = 1; i < argc; ++i) {
            if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("all")) == 0)
                g_Mode = MODE_ALL;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("rebuild"))   == 0)
                g_Mode = MODE_REBUILD;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("link"))    == 0)
                g_Mode = MODE_LINK;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("clean"))   == 0)
                g_Mode = MODE_CLEAN_ENABLED;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("exec"))    == 0)
                g_Mode = MODE_EXEC;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("lines"))   == 0)
                g_Mode = MODE_LINES_COUNT;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("--quiet")) == 0)
                g_Quiet = cake_true;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("--help"))  == 0) {
                if(help) {
                    help = cake_false;
                    // TODO: faire le message d'aide
                    printf(
                        "==========[ %s ]==========\n"
                        "Rien pour le moment...\n"
                        "Pour plus d'infos, voir la page github : https://github.com/Tytraman/Cakefile\n"
                        "============================\n"
                        , PROGRAM_NAME
                    );
                    ret = cake_false;
                }
            }else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("--version")) == 0) {
                if(version) {
                    version = cake_false;
                    printf("%s x%s version %s\nCompilé le " __DATE__ " à " __TIME__ "\n", PROGRAM_NAME, (sizeof(void *) == 8 ? "64" : "86"), VERSION);
                    ret = cake_false;
                }
            }else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("--generate")) == 0) {
                if(generate) {
                    generate = cake_false;
                    if(!cake_file_exists(OPTIONS_FILENAME)) {
                        uchar defaultCakefile[] =
                            "language : c\n\n"
                            "obj_dir : obj\n"
                            "bin_dir : bin\n\n"

                            #ifdef CAKE_WINDOWS
                            "exec_name : prog.exe\n\n"
                            #else
                            "exec_name : prog\n\n"
                            #endif

                            "includes : \n"
                            "libs : \n\n"

                            "compile_options : \n"
                            "link_options : \n\n"

                            "link_libs : \n\n"
                            "auto_exec : false\n"
                            "exec_args :\n";
                        cake_fd fd = cake_fdio_open_file(OPTIONS_FILENAME, CAKE_FDIO_ACCESS_WRITE, 0, CAKE_FDIO_OPEN_CREATE_ALWAYS, CAKE_FDIO_ATTRIBUTE_NORMAL);
                        if(fd != CAKE_FDIO_ERROR_OPEN) {
                            cake_size bytes;
                            cake_fdio_write(fd, sizeof(defaultCakefile) - 1, bytes, defaultCakefile);
                            cake_fdio_close(fd);
                        }
                    }else
                        fprintf(stderr, "Il existe déjà un fichier `%s`.\n", OPTIONS_FILENAME);
                    ret = cake_false;
                }
            }else {
                if(invalid) {
                    invalid = cake_false;
                    fprintf(stderr, "Argument invalide, entre `cake --help` pour afficher l'aide.\n");
                    ret = cake_false;
                }
            }
        }
    }
    return ret;
}

cake_bool list_files_callback(Cake_String_UTF8 *filename, void *args) {
    Cake_List_String_UTF8 *srcExtension = (Cake_List_String_UTF8 *) args;
    ulonglong i;
    for(i = 0; i < srcExtension->data.length; ++i)
        if(cake_strutf8_end_with(filename, srcExtension->list[i]->bytes))
            return cake_true;
    return cake_false;
}

cake_bool list_o_files_callback(Cake_String_UTF8 *filename, void *args) {
    if(cake_strutf8_end_with(filename, ".o"))
        return cake_true;
    return cake_false;
}

void print_missing_option(const uchar *option, const uchar *defaultValue) {
    printf("Option `%s` manquante, valeur par défaut chargée: %s\n", option, defaultValue);    
}

void print_required_option(const uchar *option) {
    fprintf(stderr, "Option obligatoire manquante: `%s`\n", option);
}

cake_bool get_fileobject_elements(Cake_FileObject *config) {
    o_ProgrammingLanguage = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "programming_language"
    );
    if(o_ProgrammingLanguage == NULL || o_ProgrammingLanguage->value->length == 0) {
        const uchar *kLanguage = "programming_language";
        const uchar *dLanguage = "c";
        o_ProgrammingLanguage = cake_fileobject_get_element(config, kLanguage);
        if(o_ProgrammingLanguage == NULL)
            o_ProgrammingLanguage = cake_list_fileobject_element_add(&config->elements, kLanguage, dLanguage);
        else if(o_ProgrammingLanguage->value->length == 0)
            cake_char_array_to_strutf8(dLanguage, o_ProgrammingLanguage->value);
        else
            goto end_programming_language;
        print_missing_option(kLanguage, o_ProgrammingLanguage->value->bytes);
    }
end_programming_language:
    if(
        cake_strutf8_equals(o_ProgrammingLanguage->value, "c") ||
        cake_strutf8_equals(o_ProgrammingLanguage->value, "C")
    )
        g_ModeLanguage = C_LANGUAGE;
    else if(
        cake_strutf8_equals(o_ProgrammingLanguage->value, "cpp") ||
        cake_strutf8_equals(o_ProgrammingLanguage->value, "CPP") ||
        cake_strutf8_equals(o_ProgrammingLanguage->value, "c++") ||
        cake_strutf8_equals(o_ProgrammingLanguage->value, "C++")
    )
        g_ModeLanguage = CPP_LANGUAGE;
    else {
        fprintf(stderr, "Le langage de programmation `%s` n'est pas supporté.\n", o_ProgrammingLanguage->value->bytes);
        return cake_false;
    }

    o_ObjDir = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "obj_dir"
    );
    if(o_ObjDir == NULL || o_ObjDir->value->length == 0) {
        const uchar *kObjDir = "obj_dir";
        const uchar *dObjDir = "obj";
        o_ObjDir = cake_fileobject_get_element(config, kObjDir);
        if(o_ObjDir == NULL)
            o_ObjDir = cake_list_fileobject_element_add(&config->elements, kObjDir, dObjDir);
        else if(o_ObjDir->value->length == 0)
            cake_char_array_to_strutf8(dObjDir, o_ObjDir->value);
        else
            goto end_obj_dir;
        print_missing_option(kObjDir, o_ObjDir->value->bytes);
    }
end_obj_dir:
    cake_strutf8_add_char_array(o_ObjDir->value,
            FILE_SEPARATOR_CHAR_STR
            #ifdef CAKE_WINDOWS
            "cakefile_windows_obj"
            #else
            "cakefile_linux_obj"
            #endif
            FILE_SEPARATOR_CHAR_STR
    );

    o_BinDir = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "bin_dir"
    );
    if(o_BinDir == NULL || o_BinDir->value->length == 0) {
        const uchar *kBinDir = "bin_dir";
        const uchar *dBinDir = "bin";
        o_BinDir = cake_fileobject_get_element(config, kBinDir);
        if(o_BinDir == NULL)
            o_BinDir = cake_list_fileobject_element_add(&config->elements, kBinDir, dBinDir);
        else if(o_BinDir->value->length == 0)
            cake_char_array_to_strutf8(dBinDir, o_BinDir->value);
        else
            goto end_bin_dir;
        print_missing_option(kBinDir, o_BinDir->value->bytes);
    }
end_bin_dir:
    cake_strutf8_add_char_array(o_BinDir->value, FILE_SEPARATOR_CHAR_STR);     // bin/

    // L'option de compilateur est obligatoire
    o_Compiler = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "compiler"
    );
    if(o_Compiler == NULL || o_Compiler->value->length == 0) {
        const uchar *kCompiler = "compiler";
        o_Compiler = cake_fileobject_get_element(config, kCompiler);
        if(o_Compiler == NULL || o_Compiler->value->length == 0) {
            print_required_option(kCompiler);
            return cake_false;
        }
    }

    o_ExecName = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "exec_name"
    );
    if(o_ExecName == NULL || o_ExecName->value->length == 0) {
        const uchar *kExecName = "exec_name";
        const uchar *dExecName =
                #ifdef CAKE_WINDOWS
                "prog.exe";
                #else
                "prog";
                #endif
        o_ExecName = cake_fileobject_get_element(config, kExecName);
        if(o_ExecName == NULL)
            o_ExecName = cake_list_fileobject_element_add(&config->elements, kExecName, dExecName);
        else if(o_ExecName->value->length == 0)
            cake_char_array_to_strutf8(dExecName, o_ExecName->value);
        else
            goto end_exec_name;
        print_missing_option(kExecName, o_ExecName->value->bytes);
    }
end_exec_name:

    Cake_FileObjectContainer *cont = cake_fileobject_get_container(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "includes"
    );

    if(cont == NULL)
        cont = cake_fileobject_get_container(config, "includes");
        

    if(cont != NULL)
        o_Includes = cont->strList;
    
    cont = cake_fileobject_get_container(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "libs"
    );
    if(cont == NULL)
        cont = cake_fileobject_get_container(config, "libs");

    if(cont != NULL)
        o_Libs = cont->strList;

    o_CompileOptions = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "compile_options"
    );
    if(o_CompileOptions == NULL || o_CompileOptions->value->length == 0)
        o_CompileOptions = cake_fileobject_get_element(config, "compile_options");

    o_LinkOptions = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "link_options"
    );
    if(o_LinkOptions == NULL || o_LinkOptions->value->length == 0)
        o_LinkOptions = cake_fileobject_get_element(config, "link_options");

    o_LinkLibs = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "link_libs"
    );
    if(o_LinkLibs == NULL || o_LinkLibs->value->length == 0)
        o_LinkLibs = cake_fileobject_get_element(config, "link_libs");

    o_AutoExec = cake_fileobject_get_element(config,
        #ifdef CAKE_WINDOWS
        WINDOWS_KEY
        #else
        LINUX_KEY
        #endif
        "auto_exec"
    );
    if(o_AutoExec == NULL || o_AutoExec->value->length == 0) {
        const uchar *kAutoExec = "auto_exec";
        const uchar *dAutoExec = "false";
        o_AutoExec = cake_fileobject_get_element(config, kAutoExec);
        if(o_AutoExec == NULL)
            o_AutoExec = cake_list_fileobject_element_add(&config->elements, kAutoExec, dAutoExec);
        else if(o_AutoExec->value->length == 0) {
            cake_char_array_to_strutf8(dAutoExec, o_AutoExec->value);
        }else {
            if(
                cake_strutf8_equals(o_AutoExec->value, "true") ||
                cake_strutf8_equals(o_AutoExec->value, "y")    ||
                cake_strutf8_equals(o_AutoExec->value, "yes")  ||
                cake_strutf8_equals(o_AutoExec->value, "enabled")
            )
                g_Mode |= MODE_EXEC_ENABLED;
            goto end_auto_exec;
        }
        print_missing_option(kAutoExec, o_AutoExec->value->bytes);
    }
end_auto_exec:

    o_ExecArgs = cake_fileobject_get_element(config,
            #ifdef CAKE_WINDOWS
            WINDOWS_KEY
            #else
            LINUX_KEY
            #endif
            "exec_args"
    );
    if(o_ExecArgs == NULL || o_ExecArgs->value->length == 0)
        o_ExecArgs = cake_fileobject_get_element(config, "exec_args");
    return cake_true;
}

// TODO: vérifier les includes dans les includes
cake_bool check_includes(cake_fd srcFd, cake_fd oFd, Cake_String_UTF8 *srcFile) {
    Cake_String_UTF8 *src = cake_strutf8("");
    cake_fdio_mem_copy_strutf8(src, srcFd, CAKE_BUFF_SIZE);
    ulonglong internalIndex = 0;
    uchar *lastPtr = src->bytes, *ptr;
    while(cake_strutf8_search_from_start(src, "#inc" "lude", &internalIndex) != NULL) {
        ptr = &src->bytes[internalIndex];
        // Si le mot-clé include est trouvé
        while(1) {
            if(internalIndex == src->data.length) {
                cake_free_strutf8(src);
                return cake_false;
            }
            // Si on trouve un include perso
            if(*ptr == '"') {
                ptr++;
                internalIndex++;
                lastPtr = ptr;
                while(1) {
                    if(internalIndex == src->data.length) {
                        cake_free_strutf8(src);
                        return cake_false;
                    }   
                    if(*ptr == '"') {
                        *ptr = '\0';

                        // On vérifie si l'include a été modifié depuis la dernière compilation
                        Cake_String_UTF8 *filename = cake_strutf8("");
                        cake_strutf8_copy(filename, srcFile);
                        ulonglong slashInternalIndex = filename->data.length - 1;
                        uchar *slashPtr = cake_strutf8_search_from_end(filename, FILE_SEPARATOR_CHAR_STR, &slashInternalIndex);
                        if(slashPtr != NULL)
                            cake_strutf8_remove_from_to_internal(filename, slashInternalIndex + 2, filename->data.length);
                        cake_strutf8_add_char_array(filename, lastPtr);

                        cake_fd hFd = cake_fdio_open_file(filename->bytes, CAKE_FDIO_ACCESS_READ, CAKE_FDIO_SHARE_READ, CAKE_FDIO_OPEN_IF_EXISTS, CAKE_FDIO_ATTRIBUTE_NORMAL);
                        cake_free_strutf8(filename);
                        if(hFd != CAKE_FDIO_ERROR_OPEN) {
                            // Si le fichier inclu a été modifié plus récemment que le fichier compilé, alors on recompile
                            if(cake_fdio_compare_time(hFd, oFd, CAKE_FDIO_COMPARE_LAST_WRITE_TIME) == CAKE_FDIO_NEWER) {
                                cake_fdio_close(hFd);
                                return cake_true;
                            }
                            cake_fdio_close(hFd);
                        }

                        internalIndex++;
                        goto end_first_loop;
                    }
                    internalIndex++;
                    ptr++;
                }
            }else if(*ptr == '\n') {
                internalIndex++;
                goto end_first_loop;
            }
            ptr++;
            internalIndex++;
        }
    end_first_loop: ;
    }
    cake_free_strutf8(src);
    return cake_false;
}
