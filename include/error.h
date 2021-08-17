#ifndef __CAKE_ERROR_H__
#define __CAKE_ERROR_H__

void error_file_not_found(const char *filename);
void error_key_not_found(const char *key);
void error_create_process(const char *command);
void error_create_pipe(const char *command, const char *std);
void error_set_handle_infos();
void error_create_folder(const char *folder);
void error_open_file(const char *filename);
void error_set_time(const char *filename);
void error_use_pipe(const char *command);
void error_create_event(const char *command);
void error_create_std(const char *command, const char *std);

#endif