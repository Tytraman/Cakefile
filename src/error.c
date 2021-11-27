#include "../include/error.h"
#include "../include/global.h"

#include <stdio.h>

void error_file_not_found(const wchar_t *filename) {
    fwprintf(stderr, L"[%S] Erreur : Fichier introuvable -> %s\n", PROGRAM_NAME, filename);
}

void error_key_not_found(const wchar_t *key) {
    fwprintf(stderr, L"[%S] Erreur : Clé introuvable -> %s\n", PROGRAM_NAME, key);
}

void error_create_process(const wchar_t *command) {
    fwprintf(stderr, L"[%S] Erreur : Impossible d'exécuter cette commande (%lu) -> %s\n", PROGRAM_NAME, GetLastError(), command);
}

void error_create_pipe(const wchar_t *command, const char *std) {
    fwprintf(stderr, L"[%S] Erreur : Impossible de créer le tunnel de redirection %S du processus (%lu) -> %s\n", PROGRAM_NAME, std, GetLastError(), command);
}

void error_set_handle_infos() {
    fwprintf(stderr, L"[%S] Erreur : Impossible de définir les informations sur le HANDLE (%lu)\n", PROGRAM_NAME, GetLastError());
}

void error_create_folder(const wchar_t *folder) {
    fwprintf(stderr, L"[%S] Erreur : Impossible de créer le dossier (%lu) -> %s\n", PROGRAM_NAME, GetLastError(), folder);
}

void error_open_file(const wchar_t *filename) {
    fwprintf(stderr, L"[%S] Erreur : Impossible d'ouvrir le fichier (%lu) -> %s\n", PROGRAM_NAME, GetLastError(), filename);
}

void error_set_time(const char *filename) {
    fwprintf(stderr, L"[%S] Erreur : Impossible de changer la date de modification du fichier (%lu) -> %S\n", PROGRAM_NAME, GetLastError(), filename);
}

void error_use_pipe(const char *command) {
    fwprintf(stderr, L"[%S] Erreur : Impossible d'utiliser les tunnels de redirections pour cette commande -> %S\n", PROGRAM_NAME, command);
}

void error_create_event(const wchar_t *command) {
    fwprintf(stderr, L"[%S] Erreur : Impossible de créer l'event (%lu) -> %s\n", PROGRAM_NAME, GetLastError(), command);
}

void error_create_std(const wchar_t *command, const char *std) {
    fwprintf(stderr, L"[%S] Erreur : Impossible de créer %S (%lu) -> %s\n", PROGRAM_NAME, std, GetLastError(), command);
}

void error_execute_command(const wchar_t *command, char result) {
    switch(result) {
        case 1:
            error_create_pipe(command, STDERR);
            break;
        case 2:
            error_create_pipe(command, STDOUT);
            break;
        case 3:
            error_create_pipe(command, STDIN);
            break;
        case 4:
            error_create_event(command);
            break;
        case 5:
            error_create_std(command, STDERR);
            break;
        case 6:
            error_create_std(command, STDOUT);
            break;
        case 7:
            error_create_std(command, STDIN);
            break;
        case 8:
            error_create_process(command);
            break;
    }
}
