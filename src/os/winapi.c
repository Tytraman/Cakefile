#include "../../include/os/winapi.h"

#include "../../include/global.h"
#include "../../include/memory/memory.h"
#include "../../include/encoding/utf8.h"
#include "../../include/funcs.h"

#include <process.h>

typedef struct ThreadData {
    HANDLE event;
    HANDLE pipe;
    unsigned char *buff;
    unsigned long buffSize;
    unsigned char flags;
    char *error;
} ThreadData;

unsigned int __stdcall stdout_thread(void *pParam) {
    ThreadData *data = (ThreadData *) pParam;

    BOOL result;
    unsigned char buff[BUFF_SIZE];
    DWORD read;
    unsigned long tempSize;

    // Lecture du pipe stdout
    while(1) {
        result = ReadFile(data->pipe, buff, BUFF_SIZE, &read, NULL);
        if(!result || read == 0)
            break;

        if(data->flags & REDIRECT_STDOUT) {
            tempSize = data->buffSize;
            data->buffSize += read;
            data->buff = (unsigned char *) realloc(data->buff, data->buffSize);
            memcpy(&data->buff[tempSize], buff, read);
        }

        if(data->flags & PRINT_STD) {
            if(g_DrawProgressBar) {
                get_last_cursor_pos();
                clear_progress_bar();
            }
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buff, read, NULL, NULL);
        }

        if(g_DrawProgressBar) {
            get_last_cursor_pos();
            draw_progress_bar(g_CurrentCompile + 1, g_NeedCompileNumber, g_ProgressBarWidthScale, g_ProgressBarFillChar, g_ProgressBarEmptyChar);
        }
    }
    SetEvent(data->event);
    _endthreadex(0);
}

unsigned int __stdcall stderr_thread(void *pParam) {
    ThreadData *data = (ThreadData *) pParam;

    BOOL result;
    unsigned char buff[BUFF_SIZE];
    DWORD read;
    unsigned long tempSize;

    // Lecture du pipe stderr
    while(1) {
        result = ReadFile(data->pipe, buff, BUFF_SIZE, &read, NULL);
        if(!result || read == 0)
            break;

        if(data->error) *data->error = 1;

        if(data->flags & REDIRECT_STDERR) {
            tempSize = data->buffSize;
            data->buffSize += read;
            data->buff = (unsigned char *) realloc(data->buff, data->buffSize);
            memcpy(&data->buff[tempSize], buff, read);
        }

        if(data->flags & PRINT_STD) {
            if(g_DrawProgressBar) {
                get_last_cursor_pos();
                clear_progress_bar();
            }
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), buff, read, NULL, NULL);
        }

        if(g_DrawProgressBar) {
            get_last_cursor_pos();
            draw_progress_bar(g_CurrentCompile + 1, g_NeedCompileNumber, g_ProgressBarWidthScale, g_ProgressBarFillChar, g_ProgressBarEmptyChar);
        }
    }
    SetEvent(data->event);
    _endthreadex(0);
}

DWORD get_program_file_name(String_UTF16 *dest) {
    wchar_t temp[0b1111111111111111];
    DWORD length = GetModuleFileNameW(NULL, temp, 0b1111111111111111);
    string_utf16_set_value(dest, temp);
    return length;
}

DWORD get_current_process_location(String_UTF16 *dest) {
    wchar_t temp[0b1111111111111111];
    DWORD length = GetCurrentDirectoryW(0b1111111111111111, temp);
    temp[length] = L'\\';
    temp[length + 1] = L'\0';
    string_utf16_set_value(dest, temp);
    return length;
}

char execute_command(wchar_t *command, String_UTF16 *out, String_UTF16 *err, char *error, unsigned char flags) {
    if(error) *error = 0;

    if(flags & REDIRECT_STDOUT && out != NULL)
        create_string_utf16(out);
    if(flags & REDIRECT_STDERR && err != NULL)
        create_string_utf16(err);

    ThreadData dataStdout = { 0 };
    ThreadData dataStderr = { 0 }; 

    dataStdout.flags = flags;
    dataStderr.flags = flags;  

    dataStderr.error = error;

    // Création des pipes anonymes :
    HANDLE childStdin_rd = NULL,  childStdin_wr = NULL;
    HANDLE childStdout_wr = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE childStderr_wr = GetStdHandle(STD_ERROR_HANDLE);

    SECURITY_ATTRIBUTES sa = { 0 };
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // stdin
    if(!CreatePipe(&childStdin_rd, &childStdin_wr, &sa, BUFF_SIZE))
        return -1;
    if(!SetHandleInformation(childStdin_wr, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(childStdin_rd);
        CloseHandle(childStdin_wr);
        return -2;
    }
    
    // stdout
    if(flags & REDIRECT_STDOUT) {
        dataStdout.event = CreateEventA(NULL, TRUE, FALSE, NULL);
        if(!CreatePipe(&dataStdout.pipe, &childStdout_wr, &sa, BUFF_SIZE)) {
            CloseHandle(childStdin_rd);
            CloseHandle(childStdin_wr);
            return -3;
        }
        if(!SetHandleInformation(dataStdout.pipe, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(childStdin_rd);
            CloseHandle(childStdin_wr);
            CloseHandle(dataStdout.pipe);
            CloseHandle(childStdout_wr);
            return -4;
        }
    }
    

    // stderr
    if(flags & REDIRECT_STDERR) {
        dataStderr.event = CreateEventA(NULL, TRUE, FALSE, NULL);
        if(!CreatePipe(&dataStderr.pipe, &childStderr_wr, &sa, BUFF_SIZE)) {
            CloseHandle(childStdin_rd);
            CloseHandle(childStdin_wr);
            CloseHandle(dataStdout.pipe);
            CloseHandle(childStdout_wr);
            return -5;
        }
        if(!SetHandleInformation(dataStderr.pipe, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(childStdin_rd);
            CloseHandle(childStdin_wr);
            CloseHandle(dataStdout.pipe);
            CloseHandle(childStdout_wr);
            CloseHandle(dataStderr.pipe);
            CloseHandle(childStderr_wr);
            return -6;
        }
    }
    
    // Copie de la commande parce que CreateProcess modifie en interne la commande.
    size_t commandLength = wcslen(command);
    wchar_t *commandCopy = (wchar_t *) malloc(commandLength * sizeof(wchar_t) + sizeof(wchar_t));
    memcpy(commandCopy, command, commandLength * sizeof(wchar_t) + sizeof(wchar_t));


    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFOW si = { 0 };
    si.cb = sizeof(si);
    si.hStdInput  = childStdin_rd;
    si.hStdOutput = childStdout_wr;
    si.hStdError  = childStderr_wr;
    si.dwFlags    = STARTF_USESTDHANDLES;

    if(!CreateProcessW(
        NULL,           // Nom de l'application
        commandCopy,    // Ligne de commande
        NULL,           // Attributs de sécurité du process
        NULL,           // Attributs de sécurité du thread
        TRUE,           // Est-ce que le process hérite des handles
        0,              // Flags
        NULL,           // Environnement du process
        NULL,           // pwd du process
        &si,            // STARTUPINFO
        &pi             // PROCESS_INFORMATION
    ))
    {
        CloseHandle(childStdin_rd);
        CloseHandle(childStdin_wr);
        CloseHandle(dataStdout.pipe);
        CloseHandle(childStdout_wr);
        CloseHandle(dataStderr.pipe);
        CloseHandle(childStderr_wr);
        free(commandCopy);
        return -7;
    }

    CloseHandle(childStdin_rd);
    CloseHandle(childStdout_wr);
    CloseHandle(childStderr_wr);

    HANDLE stdoutThread = NULL;
    HANDLE stderrThread = NULL;

    if(flags & REDIRECT_STDOUT)
        stdoutThread = (HANDLE) _beginthreadex(NULL, 0, stdout_thread, &dataStdout, 0, 0);
    if(flags & REDIRECT_STDERR)
        stderrThread = (HANDLE) _beginthreadex(NULL, 0, stderr_thread, &dataStderr, 0, 0);
    if(flags & REDIRECT_STDOUT) {
        WaitForSingleObject(dataStdout.event, INFINITE);
        CloseHandle(dataStdout.event);
        CloseHandle(stdoutThread);
        CloseHandle(dataStdout.pipe);
    }
    if(flags & REDIRECT_STDERR) {
        WaitForSingleObject(dataStderr.event, INFINITE);
        CloseHandle(dataStderr.event);
        CloseHandle(stderrThread);
        CloseHandle(dataStderr.pipe);
    }

    if(!(flags & REDIRECT_STDOUT) && !(flags & REDIRECT_STDERR))
        WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Stockage de stdout dans le String_UTF16
    if(out != NULL) {
        dataStdout.buff = (unsigned char *) realloc(dataStdout.buff, dataStdout.buffSize + 1);
        dataStdout.buff[dataStdout.buffSize] = '\0';
        String_UTF8 utf8;
        create_string_utf8(&utf8);
        array_char_to_string_utf8(dataStdout.buff, &utf8, dataStdout.buffSize + 1);
        string_utf8_to_utf16(&utf8, out);
        free(utf8.bytes);
    }

    // Stockage de stderr dans le String_UTF16
    if(err != NULL) {
        dataStderr.buff = (unsigned char *) realloc(dataStderr.buff, dataStderr.buffSize + 1);
        dataStderr.buff[dataStderr.buffSize] = '\0';
        String_UTF8 utf8;
        create_string_utf8(&utf8);
        array_char_to_string_utf8(dataStderr.buff, &utf8, dataStderr.buffSize + 1);
        string_utf8_to_utf16(&utf8, err);
        free(utf8.bytes);
    }
    free(dataStdout.buff);
    free(dataStderr.buff);

    CloseHandle(childStdin_wr);
    free(commandCopy);
    return 0;
}

unsigned long long filetime_to_ularge(FILETIME *ft) {
    ULARGE_INTEGER uLarge;
    uLarge.HighPart = ft->dwHighDateTime;
    uLarge.LowPart  = ft->dwLowDateTime;
    return uLarge.QuadPart;
}

unsigned long long get_current_time_millis() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return filetime_to_ularge(&ft) / 10000ULL;
}
