#include "../include/funcs.h"
#include "../include/global.h"

#include <stdio.h>


void draw_progress_bar(unsigned int current, unsigned int target, short widthScale, wchar_t fillChar, wchar_t emptyChar) {
    wchar_t percent[6];

    // Il y a 4 signes pourcent car dans la librairie standard, pour afficher un pourcent, il faut mettre 2 signes,
    // si il n'y en avait que 2 dans le sprintf, une fois dans le printf, il n'y en aurait plus qu'un,
    // en en mettant 4, une fois dans le printf il y en aura 2, donc le signe s'affichera correctement.
    swprintf(percent, 6, L"%d%%%%", ((100 * current) / target));
    short percentLength = wcslen(percent) - 1;
    if(g_ScreenInfo.srWindow.Right < widthScale + 16 + percentLength)
        return;

    COORD bottom;
    bottom.X = 0;
    bottom.Y = g_ScreenInfo.srWindow.Bottom;

    WORD recoveryAttributes = g_ScreenInfo.wAttributes;
    
    // Formule pour calculer la longueur de la barre d'avancement.
    short number = (((100.0f * (float) current) / (float) target) / 100.0f) * (float) widthScale;

    unsigned int i;

    const wchar_t *text = L"Compilation: ";
    wchar_t *progress = (wchar_t *) malloc(number * sizeof(wchar_t) + sizeof(wchar_t));
    for(i = 0; i < number; i++)
        progress[i] = fillChar;
    progress[i] = L'\0';

    short emptLength = widthScale - number;
    wchar_t *empt = (wchar_t *) malloc(emptLength * sizeof(wchar_t) + sizeof(wchar_t));
    for(i = 0; i < emptLength; i++)
        empt[i] = emptyChar;
    empt[i] = L'\0';

    SetConsoleCursorPosition(g_Out, bottom);
    
    if(g_LastY == bottom.Y) {
        wchar_t *emptiness = (wchar_t *) malloc(g_ScreenInfo.srWindow.Right * sizeof(wchar_t) + sizeof(wchar_t) * 2);
        for(i = 0; i < g_ScreenInfo.srWindow.Right; i++)
            emptiness[i] = L' ';
        emptiness[i] = L'\0';
        wprintf(emptiness);
        GetConsoleScreenBufferInfo(g_Out, &g_ScreenInfo);
        bottom.Y = g_ScreenInfo.srWindow.Bottom;
        SetConsoleCursorPosition(g_Out, bottom);
        wprintf(L"\n");
        GetConsoleScreenBufferInfo(g_Out, &g_ScreenInfo);
        bottom.Y = g_ScreenInfo.srWindow.Bottom;
        SetConsoleCursorPosition(g_Out, bottom);
        g_LastY = bottom.Y - 1;
        free(emptiness);
    }
    
    SetConsoleTextAttribute(g_Out, 1);
    wprintf(text);
    SetConsoleTextAttribute(g_Out, recoveryAttributes);
    wprintf(L"[");
    SetConsoleTextAttribute(g_Out, 2);
    wprintf(progress);
    SetConsoleTextAttribute(g_Out, recoveryAttributes);
    wprintf(empt);
    wprintf(L"] ");
    wprintf(percent);
    fflush(stdout);
    free(progress);
    free(empt);
    bottom.X = g_LastX;
    bottom.Y = g_LastY;
    SetConsoleCursorPosition(g_Out, bottom);
}

void clear_progress_bar() {
    unsigned int i;
    wchar_t *emptiness = (wchar_t *) malloc(g_ScreenInfo.srWindow.Right * sizeof(wchar_t) + sizeof(wchar_t) * 2);
    for(i = 0; i < g_ScreenInfo.srWindow.Right; i++)
        emptiness[i] = L' ';
    emptiness[i] = L'\0';
    COORD bottom;
    bottom.X = 0;
    bottom.Y = g_ScreenInfo.srWindow.Bottom;
    SetConsoleCursorPosition(g_Out, bottom);
    wprintf(emptiness);
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
