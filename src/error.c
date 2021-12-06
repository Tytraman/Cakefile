#include "../include/error.h"
#include "../include/global.h"
#include "../include/encoding/utf8.h"

#include <stdio.h>

void error_file_not_found(const wchar_t *filename) {
    String_UTF8 error8;
    wchar_array_to_strutf8(filename, &error8);
    fprintf(stderr, "[%s] Erreur : Fichier introuvable -> %s\n", PROGRAM_NAME, error8.bytes);
    free(error8.bytes);
}

void error_key_not_found(const wchar_t *key) {
    String_UTF8 error8;
    wchar_array_to_strutf8(key, &error8);
    fprintf(stderr, "[%s] Erreur : Clé introuvable -> %s\n", PROGRAM_NAME, error8.bytes);
    free(error8.bytes);
}

void error_create_process(const wchar_t *command) {
    String_UTF8 error8;
    wchar_array_to_strutf8(command, &error8);
    fprintf(stderr, "[%s] Erreur : Impossible d'exécuter cette commande (%lu) -> %s\n", PROGRAM_NAME, GetLastError(), error8.bytes);
    free(error8.bytes);
}

void error_create_pipe(const wchar_t *command, const char *std) {
    String_UTF8 error8;
    wchar_array_to_strutf8(command, &error8);
    fprintf(stderr, "[%s] Erreur : Impossible de créer le tunnel de redirection %s du processus (%lu) -> %s\n", PROGRAM_NAME, std, GetLastError(), error8.bytes);
    free(error8.bytes);
}

void error_set_handle_infos() {
    fprintf(stderr, "[%s] Erreur : Impossible de définir les informations sur le HANDLE (%lu)\n", PROGRAM_NAME, GetLastError());
}

void error_create_folder(const wchar_t *folder) {
    String_UTF8 error8;
    wchar_array_to_strutf8(folder, &error8);
    fprintf(stderr, "[%s] Erreur : Impossible de créer le dossier (%lu) -> %s\n", PROGRAM_NAME, GetLastError(), error8.bytes);
    free(error8.bytes);
}

void error_open_file(const wchar_t *filename) {
    String_UTF8 error8;
    wchar_array_to_strutf8(filename, &error8);
    fprintf(stderr, "[%s] Erreur : Impossible d'ouvrir le fichier (%lu) -> %s\n", PROGRAM_NAME, GetLastError(), error8.bytes);
    free(error8.bytes);
}

void error_create_event(const wchar_t *command) {
    String_UTF8 error8;
    wchar_array_to_strutf8(command, &error8);
    fprintf(stderr, "[%s] Erreur : Impossible de créer l'event (%lu) -> %s\n", PROGRAM_NAME, GetLastError(), error8.bytes);
    free(error8.bytes);
}

void error_create_std(const wchar_t *command, const char *std) {
    String_UTF8 error8;
    wchar_array_to_strutf8(command, &error8);
    fprintf(stderr, "[%s] Erreur : Impossible de créer %s (%lu) -> %s\n", PROGRAM_NAME, std, GetLastError(), error8.bytes);
    free(error8.bytes);
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
