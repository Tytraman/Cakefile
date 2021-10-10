#include "../../include/os/winapi.h"

#include "../../include/global.h"
#include "../../include/memory/memory.h"
#include "../../include/encoding/utf8.h"

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
                case ERROR_IO_PENDING: {
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

char execute_command(wchar_t *command, String_UTF16 *out, String_UTF16 *err, char *error) {
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

    if(out)
        create_string_utf16(out);
    if(err)
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
            switch(GetLastError()) {
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
                            switch(GetLastError()) {
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
                if(err){
                    tempSize = errBuffSize;
                    errBuffSize += bytesRead;
                    errBuff = (unsigned char *) realloc(errBuff, errBuffSize);
                    memcpy(&errBuff[tempSize], buff, bytesRead);
                }else
                    WriteFile(GetStdHandle(STD_ERROR_HANDLE), buff, bytesRead, NULL, NULL);
            }
        }else if(std[currentResult] == std[1]) {
            if(bytesRead > 0) {
                if(out) {
                    tempSize = outBuffSize;
                    outBuffSize += bytesRead;
                    outBuff = (unsigned char *) realloc(outBuff, outBuffSize);
                    memcpy(&outBuff[tempSize], buff, bytesRead);
                }else
                    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buff, bytesRead, NULL, NULL);
            }
        }
        overlapped.Offset += bytesRead;
    }

    if(err) {
        errBuff = (unsigned char *) realloc(errBuff, errBuffSize + 1);
        errBuff[errBuffSize] = '\0';
        String_UTF8 utf8;
        create_string_utf8(&utf8);
        array_char_to_string_utf8(errBuff, &utf8, errBuffSize + 1);
        string_utf8_to_utf16(&utf8, err);
        free(utf8.bytes);
        free(errBuff);
    }
    if(out) {
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
