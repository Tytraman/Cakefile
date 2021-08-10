#ifndef __CAKE_UTILS_H__
#define __CAKE_UTILS_H__

#include <stdio.h>
#include <windows.h>

typedef struct Array_Char {
    unsigned long length;
    char *array;
} Array_Char;


/*
    Supprime une partie d'un tableau dynamique.

    `array`, pointeur void du tableau à réduire, un cast (void **) sera sûrement nécessaire.
    `start`, pointeur d'un élément de `array` désignant le début de la partie à retirer.
    `elements`, nombre d'éléments à retirer, par exemple, si je veux retirer 5 int, je met 5.
    `byteSize`, la taille d'un octet, par exemple, si je veux enlever des int, je met `sizeof(int)`.
    `arraySize`, un pointeur de la taille totale du tableau, ne peut pas être NULL.
*/
void rem_allocate(void **array, void *start, size_t elements, size_t byteSize, unsigned long *arraySize);

/*
    Ajoute des données dans un tableau dynamique à la position indiquée.

    `array`, pointeur void du tableau auquel ajouté, un cast (void **) sera sûrement nécessaire.
    `pToAdd`, pointeur de la position à laquelle ajouter.
    `src`, tableau source à ajouter.
    `elements`, nombre d'éléments à ajouter, par exemple, si je veux ajouter 8 short, je met 8.
    `byteSize`, la taille d'un octet, par exemple, si je veux ajouter des short, je met `sizeof(short)`.
    `arraySize`, un pointeur de la taille totale du tableau, ne peut pas être NULL.
*/
void move_allocate(void **array, void *pToAdd, void *src, size_t elements, size_t byteSize, unsigned long *arraySize);

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
unsigned char *get_key_value(const char *key, unsigned char *fileBuffer, long fileSize, unsigned long *valueSize);

// Copie la valeur d'une clé récupérée grâce à get_key_value().
void copy_value(char **buffer, unsigned char *src, long valueSize);

// Réalloue `str` à 1 octet et le définie avec '\\0'
void empty_str(char **str);

/*
    Exécute la commande et stock le résultat dans`out` ou `err`.

    `error` si non NULL, contiendra 0 si le flux d'erreur est vide, sinon elle contiendra 1.

    Retourne 0 en cas de succès, sinon :
    - 1 quand le process n'a pas pu être créé,
    - 2 quand les données n'ont pas pu être lues.
*/
char execute_command(char *command, Array_Char *out, Array_Char *err, char *error);

// Fait une liste des fichiers de la commande `dir` pour pouvoir les énumérer.
unsigned long list_files(Array_Char ***dest, Array_Char *files);

void free_list(Array_Char ***list, unsigned long size);

unsigned long list_o_files(Array_Char ***dest, Array_Char *cFiles);

// Retire la partie absolue d'un chemin d'accès, ça ne converti pas un chemin, c'est uniquement de la suppression.
void relative_path(Array_Char *path);

/*
    Crée les dossiers si non existants.

    Retourne 0 en cas de succès, sinon :
    - 1 si le dossier existe déjà.
    - 2 si un dossier n'a pas pu être créé à cause des droits.
*/
int mkdirs(char *path);

/*
    Recherche des données dans une partie de la mémoire.
    Fonction universelle, on peut rechercher n'importe quoi n'importe où.

    Utile pour chercher une chaîne de caractères dans une autre chaîne, trouver le format d'un fichier grâce à son en-tête (header, magic number).

    Retourne un pointeur vers le début de l'élément trouvé, par exemple, si je cherche "phone" dans la chaîne "téléphone", la
    fonction retournera un pointeur vers le 'p' de la chaîne source.
    S'arrête à la première occurence, s'il y a plusieurs fois la chaîne recherchée, elles ne seront pas prises en compte.

    Retourne un pointeur NULL si aucune occurence n'est trouvée.

    `src`, pointeur vers le tableau à vérifier.
    `researching`, pointeur de la / des données à chercher dans `src`.
    `fromIndex`, cette fonction considère que `src` est le premier élément d'un tableau, alors `fromIndex` est considéré comme l'index de départ de la recherche,
                 il n'est pas obligatoire de suivre la logique de cette fonction, par exemple :
                      char *mot = "téléphone";
                      search_data(src, "phone", 0UL, 9UL, 5UL); -----> revient au même que faire :
                      search_data(&src[4], "phone", 0UL, 5UL, 5UL);
    `srcSize`, le nombre d'octets que la source contient, selon la manière d'utiliser cette fonction, cette valeur peut ne pas être pareil alors que la src provient du même tableau.
    `researchSize`, le nombre d'octets que le tableau de recherche contient, valeur constante peu importe la manière d'utiliser cette fonction.

    Info complémentaire :
    Lorsque l'on passe une chaîne de caractères comme paramètre, par exemple :
    search_data(src, "test", 0UL, srcSize, 4UL);
    "Test" contient en réalité 5 octets, car le compilateur ajoute automatiquement '\\0' à la fin, dans la plupart des cas, '\\0' ne sert à rien,
    alors il est plutôt conseillé de créer un tableau en y mettant les valeurs manuellement, comme ceci : char test[4] = { 't', 'e', 's', 't' }.
*/
void *search_data(void *src, void *researching, unsigned long fromIndex, unsigned long srcSize, unsigned long researchSize);

/*
    Récupère le chemin d'accès du programme.

    Retourne la longueur totale.
*/
DWORD get_program_file_name(char **buffer);

DWORD get_current_process_location(char **buffer);

unsigned long get_last_backslash(char *filenameEnd, unsigned long filenameLength);

/*
    Crée un fichier o grâce à un fichier c.

    `error` contiendra 0 si l'objet a été créé sinon 1.

    Retourne 0 en cas de succès, sinon :
    - 1 quand le process n'a pas pu être créé,
    - 2 quand les données n'ont pas pu être lues.
*/
char create_object(Array_Char *cFile, Array_Char *oFile, char *error);

unsigned long long filetime_to_ularge(FILETIME *ft);

unsigned long long filetime_diff(FILETIME *ft1, FILETIME *ft2);

unsigned long long get_current_time_millis();

unsigned long check_who_must_compile(unsigned long **list, Array_Char **listO, Array_Char **listC, unsigned long listOsize);

unsigned long str_replace(Array_Char *str, char toReplace, char replaceWith);
unsigned long str_replace_c(char *str, unsigned long strSize, char toReplace, char replaceWith);

void check_includes(Array_Char *fileC, Array_Char *fileO, unsigned long **list, unsigned long *listSize, unsigned long current);

void get_path(Array_Char *dest, Array_Char *filename);

#endif