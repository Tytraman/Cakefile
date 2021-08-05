#ifndef __CAKE_ERROR_H__
#define __CAKE_ERROR_H__

void error_file_not_found(const char *filename);
void error_key_not_found(const char *key);
void error_create_process(const char *command);
void error_create_pipe();
void error_set_handle_infos();
void error_create_folder(const char *folder);
void error_open_file(const char *filename);
void error_set_time(const char *filename);

#endif