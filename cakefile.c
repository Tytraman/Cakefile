#include "cakefile.h"

#include <stdio.h>
#include <stdlib.h>

#include <libcake/file.h>
#include <libcake/fdio.h>

#include "global.h"

#define WINDOWS_KEY "windows"
#define LINUX_KEY   "linux"

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
            }else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("generate")) == 0) {
                if(generate) {
                    generate = cake_false;
                    if(!cake_file_exists(OPTIONS_FILENAME)) {
                        char defaultCakefile[] =
                            "programming_language : c\n\n"
                            "compiler : gcc\n"
                            "linker : gcc\n"
                            "obj_dir : obj\n"
                            "bin_dir : bin\n\n"

                            "compile_command_format : {compiler} -c {src_file} -o {obj_file} {includes} {compile_options}\n"
                            "link_command_format : {linker} {list_obj_files} -o {exec_name} {link_libs} {link_options}\n\n"

                            #ifdef CAKE_WINDOWS
                            "exec_name : prog.exe\n\n"
                            #else
                            "exec_name : prog\n\n"
                            #endif
                            "compile_options : \n"
                            "link_options : \n"
                            "link_libs : \n\n"

                            "auto_exec : false\n"
                            "exec_args :\n"
                            "includes {\n\n}\n\n"
                            "libs {\n\n}\n";
                        cake_fd fd = cake_fdio_open_file(OPTIONS_FILENAME, CAKE_FDIO_ACCESS_WRITE, 0, CAKE_FDIO_OPEN_CREATE_ALWAYS, CAKE_FDIO_ATTRIBUTE_NORMAL);
                        if(fd != CAKE_FDIO_ERROR_OPEN) {
                            cake_fdio_write_no_ret(fd, sizeof(defaultCakefile) - 1, defaultCakefile);
                            cake_fdio_close(fd);
                        }
                    }else
                        fprintf(stderr, "Il existe déjà un fichier `%s`.\n", OPTIONS_FILENAME);
                    ret = cake_false;
                }
            }else {
                if(invalid) {
                    invalid = cake_false;
                    fprintf(stderr, "Argument invalide, entrez `"
                    #ifdef CAKE_WINDOWS
                    "%S"
                    #else
                    "%s"
                    #endif
                    " --help` pour afficher l'aide.\n", argv[0]);
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
        if(cake_strutf8_end_with(filename, (cchar_ptr) srcExtension->list[i]->bytes))
            return cake_true;
    return cake_false;
}

cake_bool list_o_files_callback(Cake_String_UTF8 *filename, void *args) {
    if(cake_strutf8_end_with(filename, ".o"))
        return cake_true;
    return cake_false;
}

void print_missing_option(const char *option, const char *defaultValue) {
    printf("Option `%s` manquante, valeur par défaut chargée: %s\n", option, defaultValue);    
}

void print_required_option(const char *option) {
    fprintf(stderr, "Option obligatoire manquante: `%s`\n", option);
}

cake_bool get_fileobject_elements(Cake_FileObject *config) {
    Cake_FileObjectContainer *sysCont = cake_fileobject_get_container(config,
        #ifdef CAKE_WINDOWS
        WINDOWS_KEY
        #else
        LINUX_KEY
        #endif
    );
    
    cchar_ptr programmingLanguageKey = "programming_language";
    o_ProgrammingLanguage = cake_fileobject_get_element_from(sysCont, programmingLanguageKey);
    if(o_ProgrammingLanguage == NULL || o_ProgrammingLanguage->value->length == 0) {
        cchar_ptr dLanguage = "c";
        o_ProgrammingLanguage = cake_fileobject_get_element(config, programmingLanguageKey);
        if(o_ProgrammingLanguage == NULL)
            o_ProgrammingLanguage = cake_list_fileobject_element_add(&config->elements, programmingLanguageKey, dLanguage);
        else if(o_ProgrammingLanguage->value->length == 0)
            cake_char_array_to_strutf8(dLanguage, o_ProgrammingLanguage->value);
        else
            goto end_programming_language;
        print_missing_option(programmingLanguageKey, (cchar_ptr) o_ProgrammingLanguage->value->bytes);
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

    cchar_ptr objDirKey = "obj_dir";
    o_ObjDir = cake_fileobject_get_element_from(sysCont, objDirKey);
    if(o_ObjDir == NULL || o_ObjDir->value->length == 0) {
        cchar_ptr dObjDir = "obj";
        o_ObjDir = cake_fileobject_get_element(config, objDirKey);
        if(o_ObjDir == NULL)
            o_ObjDir = cake_list_fileobject_element_add(&config->elements, objDirKey, dObjDir);
        else if(o_ObjDir->value->length == 0)
            cake_char_array_to_strutf8(dObjDir, o_ObjDir->value);
        else
            goto end_obj_dir;
        print_missing_option(objDirKey, (cchar_ptr) o_ObjDir->value->bytes);
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

    cchar_ptr binDirKey = "bin_dir";
    o_BinDir = cake_fileobject_get_element_from(sysCont, binDirKey);
    if(o_BinDir == NULL || o_BinDir->value->length == 0) {
        cchar_ptr dBinDir = "bin";
        o_BinDir = cake_fileobject_get_element(config, binDirKey);
        if(o_BinDir == NULL)
            o_BinDir = cake_list_fileobject_element_add(&config->elements, binDirKey, dBinDir);
        else if(o_BinDir->value->length == 0)
            cake_char_array_to_strutf8(dBinDir, o_BinDir->value);
        else
            goto end_bin_dir;
        print_missing_option(binDirKey, (cchar_ptr) o_BinDir->value->bytes);
    }
end_bin_dir:
    cake_strutf8_add_char_array(o_BinDir->value, FILE_SEPARATOR_CHAR_STR);     // bin/

    cchar_ptr compilerKey = "compiler";
    o_Compiler = cake_fileobject_get_element_from(sysCont, compilerKey);
    if(o_Compiler == NULL || o_Compiler->value->length == 0) {
        o_Compiler = cake_fileobject_get_element(config, compilerKey);
        if(o_Compiler == NULL || o_Compiler->value->length == 0) {
            print_required_option(compilerKey);
            return cake_false;
        }
    }

    cchar_ptr linkerKey = "linker";
    o_Linker = cake_fileobject_get_element_from(sysCont, linkerKey);
    if(o_Linker == NULL || o_Linker->value->length == 0) {
        o_Linker = cake_fileobject_get_element(config, linkerKey);
        if(o_Linker == NULL || o_Linker->value->length == 0) {
            print_required_option(linkerKey);
            return cake_false;
        }
    }

    cchar_ptr execNameKey = "exec_name";
    o_ExecName = cake_fileobject_get_element_from(sysCont, execNameKey);
    if(o_ExecName == NULL || o_ExecName->value->length == 0) {
        cchar_ptr dExecName =
                #ifdef CAKE_WINDOWS
                "prog.exe";
                #else
                "prog";
                #endif
        o_ExecName = cake_fileobject_get_element(config, execNameKey);
        if(o_ExecName == NULL)
            o_ExecName = cake_list_fileobject_element_add(&config->elements, execNameKey, dExecName);
        else if(o_ExecName->value->length == 0)
            cake_char_array_to_strutf8(dExecName, o_ExecName->value);
        else
            goto end_exec_name;
        print_missing_option(execNameKey, (cchar_ptr) o_ExecName->value->bytes);
    }
end_exec_name:

    cchar_ptr includesKey = "includes";
    Cake_FileObjectContainer *cont = cake_fileobject_get_container_from(sysCont, includesKey);
    if(cont == NULL)
        cont = cake_fileobject_get_container(config, includesKey);
    if(cont != NULL)
        o_Includes = cont->strList;
    
    cchar_ptr libsKey = "libs";
    cont = cake_fileobject_get_container_from(sysCont, libsKey);
    if(cont == NULL)
        cont = cake_fileobject_get_container(config, libsKey);
    if(cont != NULL)
        o_Libs = cont->strList;

    cchar_ptr compileOptionsKey = "compile_options";
    o_CompileOptions = cake_fileobject_get_element_from(sysCont, compileOptionsKey);
    if(o_CompileOptions == NULL || o_CompileOptions->value->length == 0)
        o_CompileOptions = cake_fileobject_get_element(config, compileOptionsKey);

    cchar_ptr linkOptionsKey = "link_options";
    o_LinkOptions = cake_fileobject_get_element_from(sysCont, linkOptionsKey);
    if(o_LinkOptions == NULL || o_LinkOptions->value->length == 0)
        o_LinkOptions = cake_fileobject_get_element(config, linkOptionsKey);

    cchar_ptr linkLibsKey = "link_libs";
    o_LinkLibs = cake_fileobject_get_element_from(sysCont, linkLibsKey);
    if(o_LinkLibs == NULL || o_LinkLibs->value->length == 0)
        o_LinkLibs = cake_fileobject_get_element(config, linkLibsKey);

    cchar_ptr autoExecKey = "auto_exec";
    o_AutoExec = cake_fileobject_get_element_from(sysCont, autoExecKey);
    if(o_AutoExec == NULL || o_AutoExec->value->length == 0) {
        cchar_ptr dAutoExec = "false";
        o_AutoExec = cake_fileobject_get_element(config, autoExecKey);
        if(o_AutoExec == NULL)
            o_AutoExec = cake_list_fileobject_element_add(&config->elements, autoExecKey, dAutoExec);
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
        print_missing_option(autoExecKey, (cchar_ptr) o_AutoExec->value->bytes);
    }
end_auto_exec:

    cchar_ptr execArgsKey = "exec_args";
    o_ExecArgs = cake_fileobject_get_element_from(sysCont, execArgsKey);
    if(o_ExecArgs == NULL || o_ExecArgs->value->length == 0)
        o_ExecArgs = cake_fileobject_get_element(config, execArgsKey);

    cchar_ptr compileCommandFormatKey = "compile_command_format";
    o_CompileCommandFormat = cake_fileobject_get_element_from(sysCont, compileCommandFormatKey);
    if(o_CompileCommandFormat == NULL || o_CompileCommandFormat->value->length == 0) {
        o_CompileCommandFormat = cake_fileobject_get_element(config, compileCommandFormatKey);
        if(o_CompileCommandFormat == NULL || o_CompileCommandFormat->value->length == 0) {
            print_required_option(compileCommandFormatKey);
            return cake_false;
        }
    }

    cchar_ptr linkCommandFormatKey = "link_command_format";
    o_LinkCommandFormat = cake_fileobject_get_element_from(sysCont, linkCommandFormatKey);
    if(o_LinkCommandFormat == NULL || o_LinkCommandFormat->value->length == 0) {
        o_LinkCommandFormat = cake_fileobject_get_element(config, linkCommandFormatKey);
        if(o_LinkCommandFormat == NULL || o_LinkCommandFormat->value->length == 0) {
            print_required_option(linkCommandFormatKey);
            return cake_false;
        }
    }
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
                        cake_strutf8_add_char_array(filename, (cchar_ptr) lastPtr);

                        cake_fd hFd = cake_fdio_open_file((cchar_ptr) filename->bytes, CAKE_FDIO_ACCESS_READ, CAKE_FDIO_SHARE_READ, CAKE_FDIO_OPEN_IF_EXISTS, CAKE_FDIO_ATTRIBUTE_NORMAL);
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

Cake_List_String_UTF8 *command_format(Cake_String_UTF8 *from) {
    Cake_List_String_UTF8 *format = cake_strutf8_split(from, " ");
    Cake_List_String_UTF8 *command = cake_list_strutf8();

    ulonglong i, j;
    for(i = 0; i < format->data.length; ++i) {
        if(cake_strutf8_equals(format->list[i], "{compiler}")) {
            cake_list_strutf8_add_char_array(command, (cchar_ptr) o_Compiler->value->bytes);
        }else if(cake_strutf8_equals(format->list[i], "{linker}")) {
            cake_list_strutf8_add_char_array(command, (cchar_ptr) o_Linker->value->bytes);
        }else if(cake_strutf8_equals(format->list[i], "{compile_options}")) {
            if(o_CompileOptions != NULL && o_CompileOptions->value->length > 0) {
                uchar *lastPtr = o_CompileOptions->value->bytes;
                uchar *ptr = lastPtr;
                cake_bool loop = cake_true;
                while(loop) {
                    if(ptr == &o_CompileOptions->value->bytes[o_CompileOptions->value->data.length]) {
                        loop = cake_false;
                        goto options_space;
                    }
                    if(*ptr == ' ') {
                        *ptr = '\0';
                    options_space:
                        cake_list_strutf8_add_char_array(command, (cchar_ptr) lastPtr);
                        lastPtr = ptr + 1;
                    }
                    ptr++;
                }
            }
        }else if(cake_strutf8_equals(format->list[i], "{link_options}")) {
            if(o_LinkOptions != NULL && o_LinkOptions->value->length > 0) {
                uchar *lastPtr = o_LinkOptions->value->bytes;
                uchar *ptr = lastPtr;
                cake_bool loop = cake_true;
                while(loop) {
                    if(ptr == &o_LinkOptions->value->bytes[o_LinkOptions->value->data.length]) {
                        loop = cake_false;
                        goto options_spacez;
                    }
                    if(*ptr == ' ') {
                        *ptr = '\0';
                    options_spacez:
                        cake_list_strutf8_add_char_array(command, (cchar_ptr) lastPtr);
                        lastPtr = ptr + 1;
                    }
                    ptr++;
                }
            }
        }else if(cake_strutf8_equals(format->list[i], "{includes}")) {
            if(o_Includes != NULL) {
                for(j = 0; j < o_Includes->data.length; ++j) {
                    #ifdef CAKE_UNIX
                    cake_strutf8_remove_all(o_Includes->list[j], "\"");
                    cake_strutf8_remove_all(o_Includes->list[j], "'");
                    #endif
                    cake_list_strutf8_add_char_array(command, (cchar_ptr) o_Includes->list[j]->bytes);
                }
            }
        }else if(cake_strutf8_equals(format->list[i], "{libs}")) {
            if(o_Libs != NULL) {
                for(j = 0; j < o_Libs->data.length; ++j) {
                    #ifdef CAKE_UNIX
                    cake_strutf8_remove_all(o_Libs->list[j], "\"");
                    cake_strutf8_remove_all(o_Libs->list[j], "'");
                    #endif
                    cake_list_strutf8_add_char_array(command, (cchar_ptr) o_Libs->list[j]->bytes);
                }
            }
        }else if(cake_strutf8_equals(format->list[i], "{exec_name}")) {
            cake_list_strutf8_add_char_array(command, (cchar_ptr) o_ExecName->value->bytes);
        }else if(cake_strutf8_equals(format->list[i], "{link_libs}")) {
            if(o_LinkLibs != NULL && o_LinkLibs->value->length > 0) {
                uchar *lastPtr = o_LinkLibs->value->bytes;
                uchar *ptr = lastPtr;
                cake_bool loop = cake_true;
                while(loop) {
                    if(ptr == &o_LinkLibs->value->bytes[o_LinkLibs->value->data.length]) {
                        loop = cake_false;
                        goto options_spacex;
                    }
                    if(*ptr == ' ') {
                        *ptr = '\0';
                    options_spacex:
                        cake_list_strutf8_add_char_array(command, (cchar_ptr) lastPtr);
                        lastPtr = ptr + 1;
                    }
                    ptr++;
                }
            }
        }else {
            cake_list_strutf8_add_char_array(command, (cchar_ptr) format->list[i]->bytes);
        }
    }

    cake_free_list_strutf8(format);
    return command;
}

void command_index(Cake_UlonglongArray *dest, Cake_List_String_UTF8 *command, const char *value) {
    dest->array = NULL;
    dest->length = 0;
    ulonglong j;
    for(j = 0; j < command->data.length; ++j) {
        if(cake_strutf8_equals(command->list[j], value)) {
            void *ptr = realloc(dest->array, (dest->length + 1) * sizeof(*dest->array));
            if(ptr != NULL) {
                dest->array = (ulonglong *) ptr;
                dest->array[dest->length] = j;
                (dest->length)++;
            }
        }
    }
}

void command_replace_index(Cake_List_String_UTF8 *command, Cake_UlonglongArray *indexArray, Cake_List_String_UTF8 *replacement, ulonglong replacementIndex) {
    ulonglong j;
    for(j = 0; j < indexArray->length; ++j) {
        cake_strutf8_copy(command->list[indexArray->array[j]], replacement->list[replacementIndex]);
        #ifdef CAKE_WINDOWS
        cake_strutf8_insert_char_array(command->list[indexArray->array[j]], 0, "\"");
        cake_strutf8_add_char_array(command->list[indexArray->array[j]], "\"");
        #endif  
    }
}

void command_replace_list(Cake_List_String_UTF8 *command, Cake_List_String_UTF8 *list, const char *value) {
    ulonglong i, j;
    for(i = 0; i < command->data.length;) {
        if(cake_strutf8_equals(command->list[i], value)) {
            cake_list_strutf8_remove(command, i);
            for(j = 0; j < list->data.length; ++j) {
                cake_list_strutf8_insert(command, i, (cchar_ptr) list->list[j]->bytes);
                i++;
            }
        }else
            i++;
    }
}
