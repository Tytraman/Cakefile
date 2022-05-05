#ifndef __CAKEFILE_H__
#define __CAKEFILE_H__

#include <libcake/def.h>
#include <libcake/strutf8.h>
#include <libcake/fileobject.h>
#include <libcake/fdio.h>


/*
        Vérifie les arguments passés à la commande.

        Retourne cake_true si un mode valide est passé, sinon cake_false.

        Si cake_false est retourné, ça ne veut pas forcément dire que le programme s'est mal déroulé.
*/
cake_bool check_args(int argc, cake_char *argv[]);

cake_bool list_files_callback(Cake_String_UTF8 *filename, void *args);
cake_bool list_o_files_callback(Cake_String_UTF8 *filename, void *args);

void print_missing_option(const char *option, const char *defaultValue);
void print_required_option(const char *option);

cake_bool get_fileobject_elements(Cake_FileObject *config);

cake_bool check_includes(cake_fd srcFd, cake_fd oFd, Cake_String_UTF8 *srcFile);

Cake_List_String_UTF8 *command_format(Cake_String_UTF8 *from);

void command_index(Cake_UlonglongArray *dest, Cake_List_String_UTF8 *command, const char *value);
void command_replace_index(Cake_List_String_UTF8 *command, Cake_UlonglongArray *indexArray, Cake_List_String_UTF8 *replacement, ulonglong replacementIndex);
void command_replace_list(Cake_List_String_UTF8 *command, Cake_List_String_UTF8 *list, const char *value);

#endif