#include "../../include/os/winapi.h"

#include "../../include/global.h"
#include "../../include/memory/memory.h"
#include "../../include/encoding/utf8.h"

#include <process.h>

typedef struct ThreadData {
    HANDLE event;
    HANDLE pipe;
    unsigned char *buff;
    unsigned long buffSize;
    char mode;
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

        switch(data->mode) {
            default: break;
            case 2:
            case 0:
                tempSize = data->buffSize;
                data->buffSize += read;
                data->buff = (unsigned char *) realloc(data->buff, data->buffSize);
                memcpy(&data->buff[tempSize], buff, read);
                if(data->mode == 3) goto stdout_show;
                break;
            case 1:
            stdout_show:
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buff, read, NULL, NULL);
                break;
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

        switch(data->mode) {
            default: break;
            case 2:
            case 0:
                tempSize = data->buffSize;
                data->buffSize += read;
                data->buff = (unsigned char *) realloc(data->buff, data->buffSize);
                memcpy(&data->buff[tempSize], buff, read);
                if(data->mode == 3) goto stdout_show;
                break;
            case 1:
            stdout_show:
                WriteFile(GetStdHandle(STD_ERROR_HANDLE), buff, read, NULL, NULL);
                break;
        }
    }
    SetEvent(data->event);
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

char execute_command_ascii(char *command, Array_Char *out, Array_Char *err, char *error) {
    if(error) *error = 0;
    char pipeName[] = "\\\\.\\pipe\\cakefile_pipe";

                   // err // out // in
    HANDLE std[3] = { NULL, NULL, NULL };

    std[0] = CreateNamedPipeA(
        pipeName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE     | PIPE_READMODE_BYTE | PIPE_WAIT,
        3,
        BUFF_SIZE,
        BUFF_SIZE,
        5000,
        NULL
    );
    if(std[0] == INVALID_HANDLE_VALUE) return 1;

    std[1] = CreateNamedPipeA(
        pipeName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE     | PIPE_READMODE_BYTE | PIPE_WAIT,
        3,
        BUFF_SIZE,
        BUFF_SIZE,
        5000,
        NULL
    );
    if(std[1] == INVALID_HANDLE_VALUE) {
        CloseHandle(std[0]);
        return 2;
    }

    std[2] = CreateNamedPipeA(
        pipeName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE     | PIPE_READMODE_BYTE | PIPE_WAIT,
        3,
        BUFF_SIZE,
        BUFF_SIZE,
        5000,
        NULL
    );
    if(std[2] == INVALID_HANDLE_VALUE) {
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        return 3;
    }

    HANDLE hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if(hEvent == INVALID_HANDLE_VALUE) {
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        return 4;
    }

    SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = { 0 };

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdError = CreateFileA(
        pipeName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_SHARE_READ   | FILE_SHARE_WRITE,
        &sa,
        OPEN_EXISTING,
        0,
        NULL
    );
    if(si.hStdError == INVALID_HANDLE_VALUE) {
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        CloseHandle(hEvent);
        return 5;
    }

    si.hStdOutput = CreateFileA(
        pipeName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_SHARE_READ   | FILE_SHARE_WRITE,
        &sa,
        OPEN_EXISTING,
        0,
        NULL
    );
    if(si.hStdOutput == INVALID_HANDLE_VALUE) {
        CloseHandle(si.hStdError);
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        CloseHandle(hEvent);
        return 6;
    }

    si.hStdInput = CreateFileA(
        pipeName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_SHARE_READ   | FILE_SHARE_WRITE,
        &sa,
        OPEN_EXISTING,
        0,
        NULL
    );
    if(si.hStdInput == INVALID_HANDLE_VALUE) {
        CloseHandle(si.hStdError);
        CloseHandle(si.hStdOutput);
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        CloseHandle(hEvent);
        return 7;
    }

    if(out) {
        out->array  = NULL;
        out->length = 0UL;
    }
    if(err) {
        err->array  = NULL;
        err->length = 0UL;
    }

    if(CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(si.hStdError);
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdInput);
    }else {
        CloseHandle(hEvent);
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        CloseHandle(si.hStdError);
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdInput);
        return 8;
    }

    char buff[BUFF_SIZE];
    DWORD bytesRead = 0;
    OVERLAPPED overlapped;
    overlapped.hEvent = hEvent;
    BOOL result = FALSE;
    char currentResult = 0;
    BOOL loop = TRUE;

    //char vfr[100];

    while(loop) {
        loop = FALSE;
        result = ReadFile(std[currentResult], buff, BUFF_SIZE, &bytesRead, &overlapped);

        //sprintf(vfr, "Current STD : %d", currentResult);
        //MessageBox(NULL, vfr, "Info", MB_OK);
        if(!result) {
            switch(GetLastError()) {
                case ERROR_SUCCESS:
                case ERROR_BROKEN_PIPE:
                    if(currentResult == 2) break;
                    currentResult++;
                    loop = TRUE;
                    continue;
                case ERROR_IO_PENDING:{
                    BOOL pending = TRUE;
                    while(pending) {
                        pending = FALSE;
                        //wprintf(L"IO pending...\n");
                        result = GetOverlappedResult(std[currentResult], &overlapped, &bytesRead, FALSE);
                        if(!result) {
                            switch(GetLastError()) {
                                case ERROR_HANDLE_EOF:
                                    //wprintf(L"GetOverlappedResult found EOF\n");
                                    break;
                                case ERROR_IO_INCOMPLETE:{
                                    result = ReadFile(std[currentResult], buff, BUFF_SIZE, &bytesRead, NULL);
                                    if(!result)
                                        wprintf(L"io error : %lu\n", GetLastError());
                                    /*
                                    if(std[currentResult] == std[0]) {
                                        if(bytesRead > 0) {
                                            if(error) *error = 1;
                                            if(err)
                                                add_allocate((void **) &err->array, buff, bytesRead, 1, &err->length);
                                            else
                                                WriteFile(GetStdHandle(STD_ERROR_HANDLE), buff, bytesRead, NULL, NULL);
                                        }
                                    }else if(std[currentResult] == std[1]) {
                                        if(bytesRead > 0) {
                                            if(out)
                                                add_allocate((void **) &out->array, buff, bytesRead, 1, &out->length);
                                            else
                                                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buff, bytesRead, NULL, NULL);
                                        }
                                    }
                                    overlapped.Offset += bytesRead;
                                    */
                                    pending = TRUE;
                                    loop = TRUE;
                                    break;
                                }
                                case ERROR_BROKEN_PIPE:
                                    //MessageBox(NULL, "Test", "MessageBox", MB_OK);
                                    //wprintf(L"Broken pipe\n");
                                    break;
                                default:
                                    //wprintf(L"GetOverlappedResult unknown error : %lu\n", GetLastError());
                                    break;
                            }
                        }else {
                            //wprintf(L"Bytes read : %lu\n", bytesRead);
                            ResetEvent(overlapped.hEvent);
                            loop = TRUE;
                        }
                    }
                    break;
                }
                default:
                    wprintf(L"Unknown error : %lu\n", GetLastError());
                    break;
            }
        }else {
            //wprintf(L"Finish sync\n");
            loop = TRUE;
        }
        if(std[currentResult] == std[0]) {
            if(bytesRead > 0) {
                if(error) *error = 1;
                if(err)
                    add_allocate((void **) &err->array, buff, bytesRead, 1, &err->length);
                else
                    WriteFile(GetStdHandle(STD_ERROR_HANDLE), buff, bytesRead, NULL, NULL);
            }
        }else if(std[currentResult] == std[1]) {
            if(bytesRead > 0) {
                if(out)
                    add_allocate((void **) &out->array, buff, bytesRead, 1, &out->length);
                else
                    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buff, bytesRead, NULL, NULL);
            }
        }
        overlapped.Offset += bytesRead;
    }
    if(out) {
        out->array = (char *) realloc(out->array, out->length + 1);
        out->array[out->length] = '\0';
    }
    if(err) {
        err->array = (char *) realloc(err->array, err->length + 1);
        err->array[err->length] = '\0';
    }
    //printf("Completed ! %lu bytes read.\n\n%s\n", overlapped.Offset, fileContent);

    CloseHandle(hEvent);
    CloseHandle(std[0]);
    CloseHandle(std[1]);
    CloseHandle(std[2]);

    return 0;
}

char execute_command(wchar_t *command, String_UTF16 *out, String_UTF16 *err, char *error, char modeStdout, char modeStderr) {
    /*
            Ancien code.

            Ca utilise l' "overlapped I/O" de la WINAPI, mais pour être très franc
            j'y comprend pas grand-chose, en plus l'exécution se bloque quand la
            commande renvoie plus de caractères que BUFF_SIZE.
    */

   /*
    if(error) *error = 0;
    const wchar_t *pipeName = L"\\\\.\\pipe\\cakefile_pipe";
    HANDLE std[3] = { NULL, NULL, NULL };

    std[0] = CreateNamedPipeW(
        pipeName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE     | PIPE_READMODE_BYTE | PIPE_WAIT,
        3,
        BUFF_SIZE,
        BUFF_SIZE,
        5000,
        NULL
    );
    if(std[0] == INVALID_HANDLE_VALUE) return 1;

    std[1] = CreateNamedPipeW(
        pipeName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE     | PIPE_READMODE_BYTE | PIPE_WAIT,
        3,
        BUFF_SIZE,
        BUFF_SIZE,
        5000,
        NULL
    );
    if(std[1] == INVALID_HANDLE_VALUE) {
        CloseHandle(std[0]);
        return 2;
    }

    std[2] = CreateNamedPipeW(
        pipeName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE     | PIPE_READMODE_BYTE | PIPE_WAIT,
        3,
        BUFF_SIZE,
        BUFF_SIZE,
        5000,
        NULL
    );
    if(std[2] == INVALID_HANDLE_VALUE) {
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        return 3;
    }

    HANDLE hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if(hEvent == INVALID_HANDLE_VALUE) {
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        return 4;
    }

    SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
    STARTUPINFOW si = { 0 };
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = { 0 };

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdError = CreateFileW(
        pipeName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_SHARE_READ   | FILE_SHARE_WRITE,
        &sa,
        OPEN_EXISTING,
        0,
        NULL
    );
    if(si.hStdError == INVALID_HANDLE_VALUE) {
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        CloseHandle(hEvent);
        return 5;
    }

    si.hStdOutput = CreateFileW(
        pipeName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_SHARE_READ   | FILE_SHARE_WRITE,
        &sa,
        OPEN_EXISTING,
        0,
        NULL
    );
    if(si.hStdOutput == INVALID_HANDLE_VALUE) {
        CloseHandle(si.hStdError);
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        CloseHandle(hEvent);
        return 6;
    }

    si.hStdInput = CreateFileW(
        pipeName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_SHARE_READ   | FILE_SHARE_WRITE,
        &sa,
        OPEN_EXISTING,
        0,
        NULL
    );
    if(si.hStdInput == INVALID_HANDLE_VALUE) {
        CloseHandle(si.hStdError);
        CloseHandle(si.hStdOutput);
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        CloseHandle(hEvent);
        return 7;
    }

    if(out != NULL && out != NULL1)
        create_string_utf16(out);
    if(err != NULL && err != NULL1)
        create_string_utf16(err);

    size_t commandLength = wcslen(command);
    wchar_t *commandCopy = (wchar_t *) malloc(commandLength * sizeof(wchar_t) + sizeof(wchar_t));
    memcpy(commandCopy, command, commandLength * sizeof(wchar_t) + sizeof(wchar_t));

    if(CreateProcessW(NULL, commandCopy, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(si.hStdError);
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdInput);
        free(commandCopy);
    }else {
        CloseHandle(hEvent);
        CloseHandle(std[0]);
        CloseHandle(std[1]);
        CloseHandle(std[2]);
        CloseHandle(si.hStdError);
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdInput);
        free(commandCopy);
        return 8;
    }

    char buff[BUFF_SIZE];
    DWORD bytesRead = 0;
    OVERLAPPED overlapped;
    overlapped.hEvent = hEvent;
    BOOL result = FALSE;
    char currentResult = 0;
    BOOL loop = TRUE;

    unsigned char *outBuff = NULL;
    unsigned long outBuffSize = 0;

    unsigned char *errBuff = NULL;
    unsigned long errBuffSize = 0;

    unsigned long tempSize;
    
    while(loop) {
        loop = FALSE;
        result = ReadFile(std[currentResult], buff, BUFF_SIZE, &bytesRead, &overlapped);

        if(!result) {
            DWORD lastError = GetLastError();
            wprintf(L"LastError : %lu\n", lastError);
            switch(lastError) {
                case ERROR_SUCCESS:
                case ERROR_BROKEN_PIPE:
                    if(currentResult == 2) break;
                    currentResult++;
                    loop = TRUE;
                    continue;
                case ERROR_IO_PENDING: {
                    BOOL pending = TRUE;
                    while(pending) {
                        pending = FALSE;
                        //wprintf(L"IO pending...\n");
                        result = GetOverlappedResult(std[currentResult], &overlapped, &bytesRead, FALSE);
                        if(!result) {
                            lastError = GetLastError();
                            switch(lastError) {
                                case ERROR_HANDLE_EOF:
                                    //wprintf(L"GetOverlappedResult found EOF\n");
                                    break;
                                case ERROR_IO_INCOMPLETE:
                                    //wprintf(L"GetOverlappedResult IO incomplete\n");
                                    pending = TRUE;
                                    loop = TRUE;
                                    break;
                                case ERROR_BROKEN_PIPE:
                                    //MessageBox(NULL, "Test", "MessageBox", MB_OK);
                                    //wprintf(L"Broken pipe\n");
                                    break;
                                default:
                                    //wprintf(L"GetOverlappedResult unknown error : %lu\n", GetLastError());
                                    break;
                            }
                        }else {
                            //wprintf(L"Bytes read : %lu\n", bytesRead);
                            ResetEvent(overlapped.hEvent);
                            loop = TRUE;
                        }
                    }
                    break;
                }
                default:
                    wprintf(L"Unknown error : %lu\n", GetLastError());
                    break;
            }
        }else {
            //wprintf(L"Finish sync\n");
            loop = TRUE;
        }
        if(std[currentResult] == std[0]) {
            if(bytesRead > 0) {
                if(error) *error = 1;
                if(err != NULL){
                    if(err != NULL1) {
                        tempSize = errBuffSize;
                        errBuffSize += bytesRead;
                        errBuff = (unsigned char *) realloc(errBuff, errBuffSize);
                        memcpy(&errBuff[tempSize], buff, bytesRead);
                    }
                }else
                    WriteFile(GetStdHandle(STD_ERROR_HANDLE), buff, bytesRead, NULL, NULL);
            }
        }else if(std[currentResult] == std[1]) {
            if(bytesRead > 0) {
                if(out != NULL) {
                    if(out != NULL1) {
                        tempSize = outBuffSize;
                        outBuffSize += bytesRead;
                        outBuff = (unsigned char *) realloc(outBuff, outBuffSize);
                        memcpy(&outBuff[tempSize], buff, bytesRead);
                    }
                }else
                    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buff, bytesRead, NULL, NULL);
            }
        }
        overlapped.Offset += bytesRead;
    }


    if(err != NULL && err != NULL1) {
        errBuff = (unsigned char *) realloc(errBuff, errBuffSize + 1);
        errBuff[errBuffSize] = '\0';
        String_UTF8 utf8;
        create_string_utf8(&utf8);
        array_char_to_string_utf8(errBuff, &utf8, errBuffSize + 1);
        string_utf8_to_utf16(&utf8, err);
        free(utf8.bytes);
        free(errBuff);
    }
    if(out != NULL && out != NULL1) {
        outBuff = (unsigned char *) realloc(outBuff, outBuffSize + 1);
        outBuff[outBuffSize] = '\0';
        String_UTF8 utf8;
        create_string_utf8(&utf8);
        array_char_to_string_utf8(outBuff, &utf8, outBuffSize + 1);
        string_utf8_to_utf16(&utf8, out);
        free(utf8.bytes);
        free(outBuff);
    }

    CloseHandle(hEvent);
    CloseHandle(std[0]);
    CloseHandle(std[1]);
    CloseHandle(std[2]);
    */

    if(error) *error = 0;

    if(out != NULL)
        create_string_utf16(out);
    if(err != NULL)
        create_string_utf16(err);

    ThreadData dataStdout = { 0 };
    ThreadData dataStderr = { 0 };

    dataStdout.mode = modeStdout;
    dataStderr.mode = modeStderr;

    dataStdout.event = CreateEventA(NULL, TRUE, FALSE, NULL);
    dataStderr.event = CreateEventA(NULL, TRUE, FALSE, NULL);

    dataStderr.error = error;

    // Création des pipes anonymes :
    HANDLE childStdin_rd,  childStdin_wr;
    HANDLE childStdout_wr;
    HANDLE childStderr_wr;

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

    // stderr
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
        TRUE,           // Est-ce que le process hérite des std
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

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(childStdin_rd);
    CloseHandle(childStdout_wr);
    CloseHandle(childStderr_wr);

    HANDLE stdoutThread = (HANDLE) _beginthreadex(NULL, 0, stdout_thread, &dataStdout, 0, 0);
    HANDLE stderrThread = (HANDLE) _beginthreadex(NULL, 0, stderr_thread, &dataStderr, 0, 0);

    WaitForSingleObject(dataStdout.event, INFINITE);
    WaitForSingleObject(dataStderr.event, INFINITE);

    CloseHandle(stdoutThread);
    CloseHandle(stderrThread);

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

    if(modeStdout == 0 || modeStdout == 2)
        free(dataStdout.buff);

    if(modeStderr == 0 || modeStderr == 2)
        free(dataStderr.buff);

    CloseHandle(childStdin_wr);
    CloseHandle(dataStdout.pipe);
    CloseHandle(dataStderr.pipe);
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
