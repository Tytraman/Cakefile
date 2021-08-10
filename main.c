#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <windows.h>
#include <stdint.h>

#include "include/utils.h"
#include "include/global.h"
#include "include/error.h"


#define MODE_ALL   1
#define MODE_RESET 2
#define MODE_LINK  3
#define MODE_CLEAN 4


int main(int argc, char **argv) {
    // On vérifie que le programme est exécuté via un terminal, et pas en double cliquant dessus
    HWND consoleWnd = GetConsoleWindow();
    DWORD processID;
    GetWindowThreadProcessId(consoleWnd, &processID);
    if(GetCurrentProcessId() == processID) {
        FreeConsole();
        MessageBoxW(NULL, L"Le programme doit être exécuté depuis un terminal.", L"Erreur !", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    
    // On met la console en UTF16 pour pouvoir écrire avec de l'unicode
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);

    char mode = MODE_ALL;   // Si l'utilisateur ne spécifie pas de mode, on le met en MODE_ALL par défaut.

    // On vérifie les arguments passés si il y en a
    if(argc > 1) {
        if(strcmp(argv[1], "all") == 0)
            mode = MODE_ALL;
        else if(strcmp(argv[1], "reset") == 0)
            mode = MODE_RESET;
        else if(strcmp(argv[1], "link") == 0)
            mode = MODE_LINK;
        else if(strcmp(argv[1], "clean") == 0)
            mode = MODE_CLEAN;
        else if(strcmp(argv[1], "--help") == 0) {
            wprintf(
                L"==========%S==========\n"
                L"Lorsqu'aucun argument n'est passé, la commande est équivalente à `cake all`.\n\n"
                L"[ Arguments ]\n"
                L"> --help : affiche ce message.\n"
                L"> --version : affiche la version installée du programme.\n"
                L"> --generate : génère un fichier `Cakefile`.\n"
                L"> all : compile uniquement les fichiers sources modifiés ou dépendants d'headers modifiés, puis les link.\n"
                L"> reset : supprime tous les fichiers objets, l'exécutable et recompile tout, puis les link.\n"
                L"> clean : supprime tous les fichiers objets et l'exécutable.\n\n"
                L"Le compilateur utilisé est gcc, alors assure toi de l'avoir installé et d'avoir la variable d'environnement !\n"
                L"Les options suivantes doivent être mises dans un fichier `Cakefile` (sans extension).\n"
                L"Le format est clé : valeur\n\n"
                L"Liste des options :\n"
                L"[ Obligatoires ]\n"
                L"- src_dir : dossier où se situent les fichiers sources.\n"
                L"- obj_dir : dossier dans lequel les fichiers compilés logeront.\n"
                L"- bin_dir : dossier dans lequel le fichier exécutable prendra ses aises.\n\n"
                L"[ Optionnelles ]\n"
                L"- exec_name : nom du fichier exécutable final (par défaut : prog.exe)\n"
                L"- includes : liste des dossiers includes externes à inclure.\n"
                L"- libs : liste des dossiers de librairies externes à inclure.\n"
                L"- compile_options : options utilisées pendant la compilation.\n"
                L"- link_options : options utilisées pendant le link des fichiers objets.\n"
                L"- link_l : librairies externes à inclure.\n\n"
                L"[ Bugs connus ]\n"
                L"> Lorsque `compile_options` contient les valeurs \"-fdata-sections\" et \"-ffunction-sections\", il faut aussi ajouter \"-Os\", \"-s\" et \"-flto\", "
                L"et ajouter dans `link_options` \"-Wl,--gc-sections\" et \"-flto\" sinon le programme se bloque au moment du link.\n"
                L"========================\n"
                , PROGRAM_NAME
            );
            return EXIT_SUCCESS;
        }else if(strcmp(argv[1], "--version") == 0) {
            wprintf(L"%S x%S version %S\n", PROGRAM_NAME, (sizeof(void *) == 8 ? "64" : "86"), VERSION);
            return EXIT_SUCCESS;
        }else if(strcmp(argv[1], "--generate") == 0) {
            char cakefile[] = "Cakefile";
            if(GetFileAttributesA(cakefile) == 0xffffffff) {
                unsigned char defaultCakefile[] =
                    "src_dir : src\n"
                    "obj_dir : obj\n"
                    "bin_dir : bin\n\n"

                    "exec_name : prog.exe\n\n"

                    "includes : \n"
                    "libs : \n\n"

                    "compile_options : \n"
                    "link_options : \n\n"

                    "link_l : ";
                FILE *pCakefile = fopen(cakefile, "wb");
                fwrite(defaultCakefile, 1, 131, pCakefile);
                fclose(pCakefile);
            }else
                wprintf(L"[%S] Il existe déjà un fichier `%S`.\n", PROGRAM_NAME, cakefile);
            return EXIT_SUCCESS;
        }else {
            wprintf(L"[%S] Argument invalide, entre `cake --help` pour afficher l'aide.", PROGRAM_NAME);
            return EXIT_FAILURE;
        }
    }

    programFilename.length = get_program_file_name(&programFilename.array);
    pwd.length = get_current_process_location(&pwd.array);

    FILE *pFile = fopen("Cakefile", "rb");
    if(!pFile) {
        error_file_not_found("Cakefile");
        return EXIT_FAILURE;
    }
    
    // Copie du fichier de configuration :
    unsigned char buff[BUFF_SIZE];

    long fileSize = get_file_size(pFile);
    unsigned char *fileBuffer = malloc(fileSize + 1);
    size_t read, total = 0;

    do {
        read = fread(buff, 1, BUFF_SIZE, pFile);
        memcpy(&fileBuffer[total], buff, read);
        total += read;
    }while(read > 0);

    fileBuffer[fileSize] = '\0';

    fclose(pFile);

    // Récupération des paramètres :

    srcDir.array  = NULL;
    srcDir.length = 0UL;

    objDir.array  = NULL;
    objDir.length = 0UL;

    binDir.array  = NULL;
    binDir.length = 0UL;

    includes.array  = NULL;
    includes.length = 0UL;

    libs.array  = NULL;
    libs.length = 0UL;

    compileOptions.array  = NULL;
    compileOptions.length = 0UL;

    linkOptions.array  = NULL;
    linkOptions.length = 0UL;

    linkLibs.array  = NULL;
    linkLibs.length = 0UL;

    unsigned char *temp;

    char key1[] = "src_dir";
    char key2[] = "obj_dir";
    char key3[] = "bin_dir";
    char key4[] = "exec_name";
    char key5[] = "includes";
    char key6[] = "libs";
    char key7[] = "compile_options";
    char key8[] = "link_options";
    char key9[] = "link_l";

    if((temp = get_key_value(key1, fileBuffer, fileSize, &srcDir.length)))
        copy_value(&srcDir.array, temp, srcDir.length);
    else {
        error_key_not_found(key1);
        goto program_end;
    }

    if((temp = get_key_value(key2, fileBuffer, fileSize, &objDir.length)))
        copy_value(&objDir.array, temp, objDir.length);
    else {
        error_key_not_found(key2);
        goto program_end;
    }

    if((temp = get_key_value(key3, fileBuffer, fileSize, &binDir.length)))
        copy_value(&binDir.array, temp, binDir.length);
    else {
        error_key_not_found(key3);
        goto program_end;
    }

    unsigned long execNameLength;
    char *execName = NULL;
    if((temp = get_key_value(key4, fileBuffer, fileSize, &execNameLength)))
        copy_value(&execName, temp, execNameLength);
    else {
        execName = malloc(9);
        memcpy(execName, "prog.exe", 9);
    }

    if((temp = get_key_value(key5, fileBuffer, fileSize, &includes.length)))
        copy_value(&includes.array, temp, includes.length);
    else
        empty_str(&includes.array);

    if((temp = get_key_value(key6, fileBuffer, fileSize, &libs.length)))
        copy_value(&libs.array, temp, libs.length);
    else
        empty_str(&libs.array);

    if((temp = get_key_value(key7, fileBuffer, fileSize, &compileOptions.length)))
        copy_value(&compileOptions.array, temp, compileOptions.length);
    else
        empty_str(&compileOptions.array);

    if((temp = get_key_value(key8, fileBuffer, fileSize, &linkOptions.length)))
        copy_value(&linkOptions.array, temp, linkOptions.length);
    else
        empty_str(&linkOptions.array);

    if((temp = get_key_value(key9, fileBuffer, fileSize, &linkLibs.length)))
        copy_value(&linkLibs.array, temp, linkLibs.length);
    else
        empty_str(&linkLibs.array);

    stdoutParent = GetStdHandle(STD_OUTPUT_HANDLE);
    stderrParent = GetStdHandle(STD_ERROR_HANDLE);


    // Création des tunnels de redirection :
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if(!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        error_create_pipe(GetLastError());
        goto program_end;
    }
    if(!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        error_create_pipe(GetLastError());
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        goto program_end;
    }
    if(!SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0)) {
        error_set_handle_infos(GetLastError());
        goto close_handles;
    }
    if(!SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0)) {
        error_set_handle_infos(GetLastError());
        goto close_handles;
    }

    Array_Char dirCout, dirCerr;
    Array_Char dirHout, dirHerr;
    dirCout.array = NULL; dirCerr.array = NULL;
    dirHout.array = NULL; dirHerr.array = NULL;

    char dirC[] = "cmd /C dir *.c /b/s";
    char result;
    if((result = execute_command(dirC, &dirCout, &dirCerr, NULL)) != 0) {
        if(result == 1)
            error_create_process(dirC);
        else {
            free(dirCout.array);
            free(dirCerr.array);
            dirCout.array = NULL;
            dirCerr.array = NULL;
        }
    }

    /* Le nom de l'exe est nécessaire dans tous les modes */
    exec.length = binDir.length + execNameLength + 1;
    exec.array = malloc(exec.length);
    memcpy(exec.array, binDir.array, binDir.length);
    exec.array[binDir.length] = '\\';
    memcpy(&exec.array[binDir.length + 1], execName, execNameLength);
    exec.array[exec.length] = '\0';
    free(execName);
    execName = NULL;
    str_replace(&exec, '/', '\\');

    /* La liste des fichiers C est nécessaire que pour compiler */
    Array_Char **listC = NULL;
    unsigned long listCsize = 0UL;

    /* La liste des fichiers H est nécessaire que pour compiler */
    Array_Char **listH = NULL;
    unsigned long listHsize = 0UL;

    /* La liste des fichiers O est nécessaire pour linker */
    Array_Char **listO = NULL;
    unsigned long listOsize = 0UL;

    if(mode == MODE_ALL || mode == MODE_RESET || mode == MODE_LINK || mode == MODE_CLEAN)
        if(dirCout.array)
            listOsize = list_o_files(&listO, &dirCout);

    unsigned long i;

    unsigned long *needCompile = NULL;
    unsigned long needCompileNumber = 0UL;

    unsigned long compileNumber = 0UL;
    unsigned long long startTime;

    char isLink = 0;

    unsigned long cleanCommandSize = 25 + objDir.length * 2;
    char *cleanCommand = malloc(cleanCommandSize + 1);
    sprintf(cleanCommand, "cmd /C if exist %s rd /q/s %s", objDir.array, objDir.array);

    switch(mode) {
        case MODE_CLEAN:
clean:
            wprintf(L"Nettoyage...\n");
            if((result = execute_command(cleanCommand, NULL, NULL, NULL)) != 0) {
                if(result == 1) {
                    error_create_process(cleanCommand);
                    goto pre_end1;
                }
            }
            remove(exec.array);
            if(mode == MODE_RESET) goto reset;
            else goto pre_end1;
        case MODE_RESET:
            goto clean;
reset:
        case MODE_ALL:
            if(dirCout.array) {
                listCsize = list_files(&listC, &dirCout);
                free(dirCout.array);
                free(dirCerr.array);
                dirCout.array = NULL;
                dirCerr.array = NULL;
            }
            dirC[13] = 'h';
            if((result = execute_command(dirC, &dirHout, &dirHerr, NULL)) != 0) {
                if(result == 1)
                    error_create_process(dirC);
                else {
                    free(dirHout.array);
                    free(dirHerr.array);
                    dirHout.array = NULL;
                    dirHerr.array = NULL;
                }
            }
            if(dirHout.array) {
                listHsize = list_files(&listH, &dirHout);
                free(dirHout.array);
                free(dirHerr.array);
                dirHout.array = NULL;
                dirHerr.array = NULL;
            }
            if((result = mkdirs(objDir.array)) != 0)
                if(result == 2) error_create_folder(objDir.array);
            needCompileNumber = check_who_must_compile(&needCompile, listO, listC, listOsize);
            for(i = 0UL; i < needCompileNumber; i++) {
                Array_Char objPath;
                get_path(&objPath, listO[needCompile[i]]);
                if((result = mkdirs(objPath.array)) != 0)
                    if(result == 2) error_create_folder(objPath.array);
                free(objPath.array);
            }
            startTime = get_current_time_millis();
            if(needCompileNumber > 0) {
                wprintf(L"==========Compilation==========\n");
                unsigned long currentCompile;
                char compileError;
                for(currentCompile = 0UL; currentCompile < needCompileNumber; currentCompile++) {
                    if(create_object(listC[needCompile[currentCompile]], listO[needCompile[currentCompile]], &compileError) == 0 && compileError == 0)
                        compileNumber++;
                }
                wprintf(L"===============================\n\n\n");
            }
        case MODE_LINK:
            if(mode == MODE_LINK) startTime = get_current_time_millis();
            if((result = mkdirs(binDir.array)) != 0)
                if(result == 2) error_create_folder(binDir.array);
            if(mode == MODE_LINK || (compileNumber > 0 && compileNumber == needCompileNumber)) {
                wprintf(L"==========Link==========\n");
                str_replace(&includes, '/', '\\');
                str_replace(&libs, '/', '\\');
                unsigned long linkCommandSize = 11UL + exec.length + linkOptions.length + includes.length + libs.length + linkLibs.length;
                char *linkCommand = NULL;
                unsigned long i, j = 4UL;
                for(i = 0UL; i < listOsize; i++)
                    linkCommandSize += listO[i]->length + 1;

                linkCommand = malloc(linkCommandSize + 1);
                char cmdGcc[] = { 'g', 'c', 'c', ' ' };
                char space = ' ';
                char output[] = { '-', 'o', ' ' };
                memcpy(linkCommand, cmdGcc, 4);
                memcpy(&linkCommand[j], linkOptions.array, linkOptions.length);
                j += linkOptions.length;
                linkCommand[j++] = space;

                for(i = 0UL; i < listOsize; i++) {
                    memcpy(&linkCommand[j], listO[i]->array, listO[i]->length);
                    j += listO[i]->length;
                    memcpy(&linkCommand[j++], &space, 1);
                }

                memcpy(&linkCommand[j], output, 3);
                j += 3;
                memcpy(&linkCommand[j], exec.array, exec.length);
                j += exec.length;
                linkCommand[j++] = space;
                memcpy(&linkCommand[j], includes.array, includes.length);
                j += includes.length;
                linkCommand[j++] = space;
                memcpy(&linkCommand[j], libs.array, libs.length);
                j += libs.length;
                linkCommand[j++] = space;
                memcpy(&linkCommand[j], linkLibs.array, linkLibs.length);

                linkCommand[linkCommandSize] = '\0';

                wprintf(L"%S\n", linkCommand);
                char linkResult, linkError;
                if((linkResult = execute_command(linkCommand, NULL, NULL, &linkError)) != 0) {
                    if(linkResult == 1)
                        error_create_process(linkCommand);
                }else
                    isLink = 1;
                free(linkCommand);
                wprintf(L"========================\n\n\n");
            }
            break;
    }

    free(needCompile);

    unsigned long long endTime = get_current_time_millis();
    wprintf(L"==========Stats==========\n");
    switch(mode) {
        default:
            wprintf(L"Aucune stat pour ce mode...\n");
            break;
        case MODE_ALL:
        case MODE_RESET:
            if(compileNumber > 0)
                wprintf(
                    L"Fichiers compilés : %lu / %lu\n"
                    L"Compilés%s en %llu ms.\n"
                    , compileNumber, needCompileNumber, (isLink ? L" et linkés" : L""), endTime - startTime
                );
            else
                wprintf(L"Rien n'a changé...\n");
            break;
        case MODE_LINK:
            wprintf(L"Linkés en %llu ms.\n", endTime - startTime);
            break;
    }
    wprintf(L"=========================\n\n");

pre_end1:
    // Libération de la mémoire des listes de fichiers
    if(listC)
        free_list(&listC, listCsize);
    if(listH)
        free_list(&listH, listHsize);
    if(listO)
        free_list(&listO, listOsize);
close_handles:
    CloseHandle(stdoutRead);
    CloseHandle(stdoutWrite);
    CloseHandle(stderrRead);
    CloseHandle(stderrWrite);
program_end:
    free(objDir.array);
    free(binDir.array);
    free(execName);
    free(fileBuffer);
    free(includes.array);
    free(libs.array);
    free(compileOptions.array);
    free(linkOptions.array);
    free(programFilename.array);
    free(exec.array);
    free(cleanCommand);
    wprintf(L"[%S] Terminé %s\n", PROGRAM_NAME, (programStatus != 0 ? L"avec une erreur..." : L"avec succès !"));
    return (programStatus != 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
