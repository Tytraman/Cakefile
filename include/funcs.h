#ifndef __FUNCS_H__
#define __FUNCS_H__

#include <wchar.h>

void draw_progress_bar(unsigned int current, unsigned int target, short widthScale, wchar_t fillChar, wchar_t emptyChar);
void clear_progress_bar();

void get_last_cursor_pos();

char is_double_clicked();

#endif