#ifndef __CAKE_UTILS_H__
#define __CAKE_UTILS_H__

#include <stdio.h>

typedef struct Array_Char {
    unsigned long length;
    char *array;
} Array_Char;

// Ajoute des données à un tableau.
void add_allocate(void **array, void *src, size_t elements, size_t byteSize, unsigned long *arraySize);

// Permet d'avoir la taille d'un fichier en utilisant la librairie standard.
long get_file_size(FILE *file);

/*
    Récupère le pointeur vers le début de la valeur d'une clé.
    Par exemple dans :
    cc : gcc
    Un pointeur vers 'g' est retourné.

    Si la clé n'est pas trouvée ou qu'il y a un problème pour trouver la valeur,
    NULL est retourné.
*/
unsigned char *get_key_value(const char *key, unsigned char *fileBuffer, long fileSize, long *valueSize);

// Copie la valeur d'une clé récupérée grâce à get_key_value().
void copy_value(unsigned char **buffer, unsigned char *src, long valueSize);

// Réalloue `str` à 1 octet et le définie avec '\\0'
void empty_str(unsigned char **str);

/*
    Exécute la commande et stock le résultat dans`out` ou `err`.

    Retourne 0 en cas de succès, sinon :
    1 quand le process n'a pas pu être créé,
    2 quand les données n'ont pas pu être lues.

    Un appel à GetLastError() permet d'avoir le code d'erreur.
*/
char execute_command(char *command, Array_Char *out, Array_Char *err);

// Fait une liste des fichiers de la commande `dir` pour pouvoir les énumérer.
unsigned long list_files(Array_Char ***dest, Array_Char *files);

void free_list(Array_Char ***list, unsigned long size);

#endif