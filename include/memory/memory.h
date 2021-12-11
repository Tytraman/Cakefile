#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdio.h>

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

// Copie la valeur d'une clé récupérée grâce à get_key_value().
void copy_value(char **buffer, unsigned char *src, long valueSize);



#endif