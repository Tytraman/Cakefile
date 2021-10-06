# Cakefile

Reprend le principe de [**Make**](https://fr.wikipedia.org/wiki/Make), ne compile que les fichiers sources **modifiés**. Si un header a été modifié, tous les fichiers en dépandant seront **recompilés**.<br><br>
### <span style="color:#f56e92">**Caractères unicode et espaces pris en charge !**</span><br><br>
## Installation
### Uniquement sous Windows !
**Télécharge** l'exécutable ici : https://github.com/Tytraman/Cakefile/releases/latest, et tu peux soit le **copier** dans le dossier root de ton projet ou alors **l'ajouter** dans les variables d'environnement système pour y avoir accès partout.

## Utilisation
Pour utiliser la commande `cake`, un fichier `Cakefile` doit être présent dans le dossier root du projet.<br>
La commande `cake --generate` permet de générer ce fichier avec les options par défaut.<br><br>
Liste des commandes :

- **`cake clean`** : supprime le dossier contenant les fichiers `.o` et l'exécutable final.
- **`cake all`** : compile les fichiers modifiés puis crée l'exécutable.
- **`cake reset`** : équivalent de `cake clean` puis `cake all`.
- **`cake --help`** : affiche l'aide.
- **`cake --version`** : affiche la version installée du programme.

Pour éviter de taper `cake all` à chaque fois, tu peux simplement taper **`cake`**, ça fait pareil (〃▽〃)<br><br>

## Configuration
Le fichier `Cakefile` contient toutes les options que le programme utilise, c'est le **pilier principal**, sans lui, rien ne va, alors il est très important de bien comprendre comment s'en servir.<br>
En utilisant `cake`, une certaine structure de fichiers doit être respectée, les fichiers sources **DOIVENT** se situer dans le dossier root du projet **ET/OU** dans un sous-dossier contenant les fichiers sources.<br>
Voici un exemple :
```txt
E:.
│   Cakefile
│   main.c
│   README.md
│
├───bin
│       cake.exe
│
├───include
│   │   error.h
│   │   global.h
│   │
│   ├───array
│   │       base_array.h
│   │
│   └───encoding
│           utf16.h
│           utf8.h
│           
└───src
    │   error.c
    │   global.c
    │   
    ├───array
    │       base_array.c
    │       
    └───encoding
            utf16.c
            utf8.c
```
`main.c` se situe dans le dossier root, et le dossier `src` contient tous les autres fichiers `.c`, ils peuvent être dans des sous-dossiers, du moment qu'ils sont dans le dossier `src`.<br><br>

Liste des options du fichier `Cakefile` :
- **`src_dir`** : dossier contenant les fichiers sources.
- **`obj_dir`** : dossier où sont stockés les fichiers `.o` une fois les fichiers `.c` compilés.
- **`bin_dir`** : dossier où sera stocké l'exécutable final.
- **`exec_name`** : nom de l'exécutable final.
- **`includes`** : dossiers externes au projet à inclure lors de l'utilisation d'une ou plusieurs librairies. Pour ajouter plusieurs dossiers, il faut les séparer avec la barre oblique `|`. Exemple : `C:\Lib1\include|C:\Lib2\include|C:\Program Files\Lib\include`.
- **`libs`** : comme `includes`, mais pour les fichiers compilés ou sources de la librairie. Exemple : `C:\Lib1\lib|C:\Lib2\lib|C:\Program Files\Lib\lib`.
- **`compile_options`** : options à passer lors de la compilation, exemple : `-Wall`.
- **`link_options`** : options à passer lors du link, exemple : `-Wl,--gc-sections`.
- **`link_l`** : liste des librairies avec lesquelles link, exemple : `-lWs2_32 -lssl -lcrypto`.