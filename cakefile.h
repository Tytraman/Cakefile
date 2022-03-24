#ifndef __CAKEFILE_H__
#define __CAKEFILE_H__

#include "include/libcake/def.h"
#include "include/libcake/strutf8.h"
#include "include/libcake/fileobject.h"

/*
        Vérifie les arguments passés à la commande.

        Retourne cake_true si un mode valide est passé, sinon cake_false.

        Si cake_false est retourné, ça ne veut pas forcément dire que le programme s'est mal déroulé.
*/
cake_bool check_args(int argc, cake_char *argv[]);

cake_bool list_files_callback(Cake_String_UTF8 *filename);
cake_bool list_o_files_callback(Cake_String_UTF8 *filename);

void print_missing_option(const uchar *option, const uchar *defaultValue);
void print_required_option(const uchar *option);

cake_bool get_fileobject_elements(Cake_FileObject *config);

#endif