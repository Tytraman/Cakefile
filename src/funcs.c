#include "../include/funcs.h"
#include "../include/global.h"
#include "../include/encoding/utf8.h"

#include <stdio.h>


void draw_progress_bar(unsigned int current, unsigned int target, short widthScale, wchar_t fillChar, wchar_t emptyChar) {
    char percent[6];

    // Il y a 4 signes pourcent car dans la librairie standard, pour afficher un pourcent, il faut mettre 2 signes,
    // si il n'y en avait que 2 dans le sprintf, une fois dans le printf, il n'y en aurait plus qu'un,
    // en en mettant 4, une fois dans le printf il y en aura 2, donc le signe s'affichera correctement.
    snprintf(percent, 6, "%d%%%%", ((100 * current) / target));
    short percentLength = strlen(percent) - 1;
    if(g_ScreenInfo.srWindow.Right < widthScale + 16 + percentLength)
        return;

    COORD bottom;
    bottom.X = 0;
    bottom.Y = g_ScreenInfo.srWindow.Bottom;

    WORD recoveryAttributes = g_ScreenInfo.wAttributes;
    
    // Formule pour calculer la longueur de la barre d'avancement.
    short number = (((100.0f * (float) current) / (float) target) / 100.0f) * (float) widthScale;

    unsigned int i;

    const char *text = "Compilation: ";
    wchar_t *progress = (wchar_t *) malloc(number * sizeof(wchar_t) + sizeof(wchar_t));
    for(i = 0; i < number; i++)
        progress[i] = fillChar;
    progress[i] = L'\0';
    String_UTF8 progress8;
    wchar_array_to_strutf8(progress, &progress8);
    free(progress);

    short emptLength = widthScale - number;
    wchar_t *empt = (wchar_t *) malloc(emptLength * sizeof(wchar_t) + sizeof(wchar_t));
    for(i = 0; i < emptLength; i++)
        empt[i] = emptyChar;
    empt[i] = L'\0';
    String_UTF8 empt8;
    wchar_array_to_strutf8(empt, &empt8);
    free(empt);

    SetConsoleCursorPosition(g_Out, bottom);
    
    if(g_LastY == bottom.Y) {
        char *emptiness = (char *) malloc(g_ScreenInfo.srWindow.Right * sizeof(char) + sizeof(char) * 2);
        for(i = 0; i < g_ScreenInfo.srWindow.Right; i++)
            emptiness[i] = ' ';
        emptiness[i] = '\0';
        printf(emptiness);
        GetConsoleScreenBufferInfo(g_Out, &g_ScreenInfo);
        bottom.Y = g_ScreenInfo.srWindow.Bottom;
        SetConsoleCursorPosition(g_Out, bottom);
        printf("\n");
        GetConsoleScreenBufferInfo(g_Out, &g_ScreenInfo);
        bottom.Y = g_ScreenInfo.srWindow.Bottom;
        SetConsoleCursorPosition(g_Out, bottom);
        g_LastY = bottom.Y - 1;
        free(emptiness);
    }
    SetConsoleTextAttribute(g_Out, 1);
    printf(text);
    SetConsoleTextAttribute(g_Out, recoveryAttributes);
    printf("[");
    SetConsoleTextAttribute(g_Out, 2);
    printf(progress8.bytes);
    SetConsoleTextAttribute(g_Out, recoveryAttributes);
    printf(empt8.bytes);
    printf("] ");
    printf(percent);
    fflush(stdout);
    free(progress8.bytes);
    free(empt8.bytes);
    bottom.X = g_LastX;
    bottom.Y = g_LastY;
    SetConsoleCursorPosition(g_Out, bottom);
}

void clear_progress_bar() {
    unsigned int i;
    char *emptiness = (char *) malloc(g_ScreenInfo.srWindow.Right * sizeof(char) + sizeof(char) * 2);
    for(i = 0; i < g_ScreenInfo.srWindow.Right; i++)
        emptiness[i] = ' ';
    emptiness[i] = '\0';
    COORD bottom;
    bottom.X = 0;
    bottom.Y = g_ScreenInfo.srWindow.Bottom;
    SetConsoleCursorPosition(g_Out, bottom);
    printf(emptiness);
    free(emptiness);
    bottom.X = g_LastX;
    bottom.Y = g_LastY;
    SetConsoleCursorPosition(g_Out, bottom);
}

void get_last_cursor_pos() {
    GetConsoleScreenBufferInfo(g_Out, &g_ScreenInfo);
    g_LastX = g_ScreenInfo.dwCursorPosition.X;
    g_LastY = g_ScreenInfo.dwCursorPosition.Y;
}
