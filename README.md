# Cakefile

Reprend le principe de [**Make**](https://fr.wikipedia.org/wiki/Make), ne compile que les fichiers sources **modifiés**. Si un header a été modifié, tous les fichiers en dépandant seront **recompilés**.<br><br>
### <span style="color:#f56e92">**Caractères unicode et espaces pris en charge !**</span><br><br>
## Installation
### Uniquement sous Windows !
**Télécharge** l'exécutable ici : https://github.com/Tytraman/Cakefile/releases/latest, et tu peux soit le **copier** dans le dossier root de ton projet ou alors **l'ajouter** dans les variables d'environnement système pour y avoir accès partout.

<br>

## Utilisation
Pour utiliser la commande `cake`, un fichier `Cakefile` doit être présent dans le dossier root du projet.<br>
La commande `cake --generate` permet de générer ce fichier avec les options par défaut.<br><br>
Liste des commandes :

- **`cake clean`** : supprime les fichiers objets et l'exécutable final.
- **`cake all`** : compile les fichiers modifiés puis crée l'exécutable.
- **`cake reset`** : équivalent de `cake clean` puis `cake all`.
- **`cake link`** : link les fichiers objets pour créer l'exécutable.
- **`cake exec`** : exécute le programme avec les arguments dans l'option `exec_args`.
- **`cake lines`** : affiche le nombre de lignes de chaque fichier puis le total.
- **`cake --help`** : affiche l'aide.
- **`cake --version`** : affiche la version installée du programme.

Pour éviter de taper `cake all` à chaque fois, tu peux simplement taper **`cake`**, ça fait pareil (〃▽〃)

<br>

## Configuration
Liste des options du fichier `Cakefile` :

**[ Obligatoires ]**
- **`language`** : langage de programmation utilisé, pour plus d'infos voir [**les langages supportés**](#head_languages).
- **`obj_dir`** : dossier où sont stockés les fichiers `.o` une fois les fichiers `.c` compilés.
- **`bin_dir`** : dossier où sera stocké l'exécutable final.
- **`exec_name`** : nom de l'exécutable final.

**[ Optionnelles ]**
- **`includes`** : dossiers externes au projet à inclure lors de l'utilisation d'une ou plusieurs librairies. Pour ajouter plusieurs dossiers, il faut les séparer avec la barre oblique `|`. Exemple : `C:\Lib1\include|C:\Lib2\include|C:\Program Files\Lib\include`.
- **`libs`** : comme `includes`, mais pour les fichiers compilés ou sources de la librairie. Exemple : `C:\Lib1\lib|C:\Lib2\lib|C:\Program Files\Lib\lib`.
- **`compile_options`** : options à passer lors de la compilation, exemple : `-Wall`.
- **`link_options`** : options à passer lors du link, exemple : `-Wl,--gc-sections`.
- **`link_l`** : liste des librairies avec lesquelles link, exemple : `-lWs2_32 -lssl -lcrypto`.
- **`auto_exec`** : définie si le programme s'exécute automatiquement après sa génération. (*true*/*false*).
- **`exec_args`** : liste des arguments à passer si auto_exec est activé ou si la commande `cake exec` a été tapée.

<br>

## <a name="head_languages"></a> Langages de programmation
Cakefile ne fonctionne que sur des langages prédéfinis, dont voici la liste :
- **`Langage C`** : gcc
- **`C++`** : g++
