#include "cakefile.h"

#include <stdio.h>
#include <stdlib.h>

#include "include/libcake/file.h"
#include "include/libcake/fdio.h"

#include "global.h"

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
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("reset")) == 0)
                g_Mode = MODE_RESET;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("link")) == 0)
                g_Mode &= MODE_LINK_ENABLED;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("clean")) == 0)
                g_Mode &= MODE_CLEAN_ENABLED;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("exec")) == 0)
                g_Mode = MODE_EXEC;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("lines")) == 0)
                g_Mode = MODE_LINES_COUNT;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("--quiet")) == 0)
                g_Quiet = cake_true;
            else if(CAKE_CHAR_CMP(argv[i], CAKE_CHAR("--help")) == 0) {
                if(help) {
                    help = cake_false;
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
                        "- link_libs : librairies externes à inclure.\n"
                        "- auto_exec : définie si le programme s'exécute automatiquement après sa génération. (true/false)\n"
                        "- exec_args : liste des arguments à passer si auto_exec est activé ou si la commande `cake exec` a été tapée.\n\n"
                        
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

cake_bool list_files_callback(Cake_String_UTF8 *filename) {
    if(cake_strutf8_end_with(filename, ".c"))
        return cake_true;
    return cake_false;
}

cake_bool list_o_files_callback(Cake_String_UTF8 *filename) {
    if(cake_strutf8_end_with(filename, ".o"))
        return cake_true;
    return cake_false;
}

void print_missing_option(const uchar *option, const uchar *defaultValue) {
    printf("Option `%s` manquante, valeur par défaut chargée : %s\n", option, defaultValue);    
}

void print_required_option(const uchar *option) {
    fprintf(stderr, "Option obligatoire manquante : `%s`\n", option);
}

cake_bool get_fileobject_elements(Cake_FileObject *config) {
    const uchar *kLanguage = "language";
    o_Language = cake_file_object_get_element(config, kLanguage);
    if(o_Language == NULL || o_Language->value->length == 0) {
        cake_char_array_to_strutf8("c", o_Language->value);
        print_missing_option(kLanguage, o_Language->value->bytes);
    }
    if(cake_strutf8_equals(o_Language->value, "c"))
        g_ModeLanguage = C_LANGUAGE;
    else if(
        cake_strutf8_equals(o_Language->value, "cpp") ||
        cake_strutf8_equals(o_Language->value, "c++")
    )
        g_ModeLanguage = CPP_LANGUAGE;
    else {
        fprintf(stderr, "Le langage de programmation `%s` n'est pas supporté.\n", o_Language->value->bytes);
        cake_free_file_object(config);
        return cake_false;
    }

    const uchar *kObjDir = "obj_dir";
    o_ObjDir = cake_file_object_get_element(config, kObjDir);
    if(o_ObjDir == NULL || o_ObjDir->value->length == 0) {
        cake_char_array_to_strutf8("obj", o_ObjDir->value);
        print_missing_option(kObjDir, o_ObjDir->value->bytes);
    }
    cake_strutf8_add_char_array(o_ObjDir->value, FILE_SEPARATOR_CHAR_STR);  // obj/

    const uchar *kBinDir = "bin_dir";
    o_BinDir = cake_file_object_get_element(config, kBinDir);
    if(o_BinDir == NULL || o_BinDir->value->length == 0) {
        cake_char_array_to_strutf8("bin", o_BinDir->value);
        print_missing_option(kBinDir, o_BinDir->value->bytes);
    }
    cake_strutf8_add_char_array(o_BinDir->value, FILE_SEPARATOR_CHAR_STR);     // bin/

    // L'option de compilateur est obligatoire
    const uchar *kCompiler = "compiler";
    o_Compiler = cake_file_object_get_element(config, kCompiler);
    if(o_Compiler == NULL || o_Compiler->value->length == 0) {
        print_required_option(kCompiler);
        return cake_false;
    }

    const uchar *kExecName = "exec_name";
    o_ExecName = cake_file_object_get_element(config, kExecName);
    if(o_ExecName == NULL || o_ExecName->value->length == 0) {
        cake_char_array_to_strutf8(
            #ifdef CAKE_WINDOWS
            "prog.exe"
            #else
            "prog"
            #endif
            , o_ExecName->value
        );
        print_missing_option(kExecName, o_ExecName->value->bytes);
    }

    o_Includes       = cake_file_object_get_container(config, "includes");
    o_Libs           = cake_file_object_get_container(config, "libs");
    o_CompileOptions = cake_file_object_get_element(config, "compile_options");
    o_LinkOptions    = cake_file_object_get_element(config, "link_options");
    o_LinkLibs       = cake_file_object_get_element(config, "link_libs");

    const uchar *kAutoExec = "auto_exec";
    o_AutoExec = cake_file_object_get_element(config, kAutoExec);
    if(o_AutoExec == NULL || o_AutoExec->value->length == 0) {
        cake_char_array_to_strutf8("false", o_AutoExec->value);
        print_missing_option(kAutoExec, o_AutoExec->value->bytes);
    }else {
        if(
            cake_strutf8_equals(o_AutoExec->value, "true") ||
            cake_strutf8_equals(o_AutoExec->value, "y")    ||
            cake_strutf8_equals(o_AutoExec->value, "yes")  ||
            cake_strutf8_equals(o_AutoExec->value, "enabled")
        )
            g_AutoExec = cake_true;
        // g_AutoExec = cake_false dans global.c
    }

    o_ExecArgs = cake_file_object_get_element(config, "exec_args");
    return cake_true;
}
