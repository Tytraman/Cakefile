#include "../include/error.h"
#include "../include/global.h"

#include <stdio.h>

void error_file_not_found(const char *filename) {
    wprintf(L"[%S] Erreur : Fichier introuvable -> %S\n", PROGRAM_NAME, filename);
    programStatus = PROGRAM_STATUS_FILE_NOT_FOUND;
}

void error_key_not_found(const char *key) {
    wprintf(L"[%S] Erreur : Clé introuvable -> %S\n", PROGRAM_NAME, key);
    programStatus = PROGRAM_STATUS_KEY_NOT_FOUND;
}

void error_create_process(const char *command) {
    wprintf(L"[%S] Erreur : Impossible d'exécuter cette commande (%lu) -> %S\n", PROGRAM_NAME, GetLastError(), command);
    programStatus = PROGRAM_STATUS_ERROR_CREATE_PROCESS;
}

void error_create_pipe(const char *command, const char *std) {
    wprintf(L"[%S] Erreur : Impossible de créer le tunnel de redirection %S du processus (%lu) -> %S\n", PROGRAM_NAME, std, GetLastError(), command);
    programStatus = PROGRAM_STATUS_ERROR_CREATE_PIPE;
}

void error_set_handle_infos() {
    wprintf(L"[%S] Erreur : Impossible de définir les informations sur le HANDLE (%lu)\n", PROGRAM_NAME, GetLastError());
    programStatus = PROGRAM_STATUS_ERROR_SET_HANDLE_INFOS;
}

void error_create_folder(const char *folder) {
    wprintf(L"[%S] Erreur : Impossible de créer le dossier -> %S\n", folder);
}

void error_open_file(const char *filename) {
    wprintf(L"[%S] Erreur : Impossible d'ouvrir le fichier (%lu) -> %S\n", PROGRAM_NAME, GetLastError(), filename);
}

void error_set_time(const char *filename) {
    wprintf(L"[%S] Erreur : Impossible de changer la date de modification du fichier (%lu) -> %S\n", PROGRAM_NAME, GetLastError(), filename);
}

void error_use_pipe(const char *command) {
    wprintf(L"[%S] Erreur : Impossible d'utiliser les tunnels de redirections pour cette commande -> %S\n", PROGRAM_NAME, command);
}

void error_create_event(const char *command) {
    wprintf(L"[%S] Erreur : Impossible de créer l'event (%lu) -> %S\n", PROGRAM_NAME, GetLastError(), command);
}

void error_create_std(const char *command, const char *std) {
    wprintf(L"[%S] Erreur : Impossible de créer %S (%lu) -> %S\n", PROGRAM_NAME, std, GetLastError(), command);
}
