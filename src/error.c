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

void error_create_process(const char *command, unsigned long errorCode) {
    wprintf(L"[%S] Erreur : Impossible d'exécuter cette commande (%lu) -> %S\n", PROGRAM_NAME, errorCode, command);
    programStatus = PROGRAM_STATUS_ERROR_CREATE_PROCESS;
}

void error_create_pipe(unsigned long errorCode) {
    wprintf(L"[%S] Erreur : Impossible de créer le tunnel de redirection du processus (%lu)\n", PROGRAM_NAME, errorCode);
    programStatus = PROGRAM_STATUS_ERROR_CREATE_PIPE;
}

void error_set_handle_infos(unsigned long errorCode) {
    wprintf(L"[%S] Erreur : Impossible de définir les informations sur le HANDLE (%lu)\n", PROGRAM_NAME, errorCode);
    programStatus = PROGRAM_STATUS_ERROR_SET_HANDLE_INFOS;
}
