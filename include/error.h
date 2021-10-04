#ifndef __CAKE_ERROR_H__
#define __CAKE_ERROR_H__

#include <stddef.h>

void error_file_not_found(const char *filename);
void error_key_not_found(const wchar_t *key);
void error_create_process(const wchar_t *command);
void error_create_pipe(const wchar_t *command, const char *std);
void error_set_handle_infos();
void error_create_folder(const wchar_t *folder);
void error_open_file(const wchar_t *filename);
void error_set_time(const char *filename);
void error_use_pipe(const char *command);
void error_create_event(const wchar_t *command);
void error_create_std(const wchar_t *command, const char *std);
void error_execute_command(const wchar_t *command, char result);

#endif