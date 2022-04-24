# Cakefile

Reprend le principe de [**Make**](https://fr.wikipedia.org/wiki/Make), ne compile que les fichiers sources **modifiés**. Si un header a été édité, tous les fichiers en dépandant seront **recompilés**.

**La version stable n'étant pas terminée, la documentation peut changer entre temps.**

## Compilation
Dépendance : [**libcake**](https://github.com/Tytraman/libcake)<br>
`gcc *.c -lcake -o cake`

## Documentation
### Utilisation
Pour commencer, un fichier `Cakefile` doit être présent à la racine du projet.<br>
Vous pouvez générer ce fichier avec les options par défaut en exécutant la commande `cake --generate`.

Liste des commandes :
- `cake clean` : supprime les fichiers objets et le fichier de destination.
- `cake all` : compile les fichiers modifiés puis crée le fichier de destination.
- `cake rebuild` : recompile tous les fichiers, modifiés ou non puis crée le fichier de destination.
- `cake link` : link les fichiers objets pour créer le fichier de destination.
- `cake exec` : exécute le fichier destination avec les arguments dans l'option `exec_args`.
- `cake lines` : affiche le nombre de lignes de chaque fichier puis le total.

Pour éviter de taper `cake all`, vous pouvez simplement entrer `cake`, ça fait pareil (〃▽〃)

Liste des arguments optionnels :
- `--help` : affiche l'aide.
- `--version` : affiche la version de Cakefile et la date de compilation.
- `--quiet` : aucun message n'est affiché dans la console *(stdout)*.

### Configuration
Les options utilisées par le programme sont chargées depuis le fichier `Cakefile`, il est obligatoire, sans celui-ci le programme retournera une erreur.

Liste des options :

- `programming_language` : langage de programmation utilisé, pour plus d'informations voir [**les langages supportés**](#programming_languages).
- `bin_dir` : dossier dans lequel sera stocké le fichier final.
- `obj_dir` : dossier dans lequel seront stockés les fichiers objets.
- `compiler` : compilateur utilisé.
- `exec_name` : nom du fichier destination.
- `includes` : liste des dossiers à inclure lors de la compilation.
- `libs` : liste des dossiers contenant les librairies à inclure lors de la compilation.
- `link_libs` : librairies à ajouter lors du link, exemple : `-lcake -lWs2_32 -lssl -lcrypto`.
- `auto_exec` : définie si l'exécutable final doit être automatiquement exécuté.
- `exec_args` : arguments à passer lorsque la commande `cake exec` est entrée ou que `auto_exec` est activée.

Si vous développez sur plusieurs plateformes, vous pouvez utiliser des options différentes pour chacun des systèmes, actuellement les 2 seuls disponibles sont `linux` et `windows`.<br>
Voici un exemple d'un fichier `Cakefile` :
```
bin_dir : bin
obj_dir : obj
programming_language : c
auto_exec : false
exec_name : prog

linux {
    compiler : gcc
    exec_name : linux_prog
    link_libs : -ltest1 -ltest2
    includes {
        - /home/me/mylib/include
        - /media/USB/include
    }
    libs {
        - /home/me/mylib/lib
        - /media/USB/lib
    }
}

windows {
    compiler : g++
    link_options : -municode
    exec_name : windows_prog.exe
}

```
Les paramètres spécifiés dans `linux` et `windows` viennent écraser ceux qui sont globaux. Dans l'exemple ci-dessus, `exec_name` est défini de manière globale avec la valeur *prog*, mais vu que l'option est redéfinie dans `linux` et `windows`, cette valeur ne sera prise en compte par aucun des systèmes.

## <a name="programming_languages"></a> Langages de programmation
Cakefile ne fonctionne que sur des langages prédéfinis, dont voici la liste :
- [**Langage C**](https://fr.wikipedia.org/wiki/C_(langage))
- [**C++**](https://fr.wikipedia.org/wiki/C%2B%2B)