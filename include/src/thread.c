#include "../thread.h"

#ifdef _WIN32
unsigned int __stdcall win32_thread(void *pParam) {
    Thread *thread = (Thread *) pParam;
    unsigned int returnValue = thread->runCallback(thread->args.args);
    if(thread->endCallback)
        thread->endCallback();
    thread_exit(0);
    return returnValue;
}
#endif

void create_thread(Thread *thread, ThreadRunCallback runCallback, ThreadEndCallback endCallback) {
    thread->hThread  = NULL;
    thread->runCallback = runCallback;
    thread->endCallback = endCallback;
    thread->args.args = NULL;
    thread->args.size = 0;
}

char thread_run(Thread *thread) {
    if(thread->runCallback == NULL) return THREAD_NO_RUN_CALLBACK;
    #ifdef _WIN32
    thread->hThread = (HANDLE) _beginthreadex(NULL, 0, win32_thread, thread, 0, 0);
    #endif
    return THREAD_OK;
}

void thread_wait(Thread *thread) {
    #ifdef _WIN32
    WaitForSingleObject(thread->hThread, INFINITE);
    #endif
}

void thread_set_args(Thread *thread, void *pArgs, unsigned long long size) {
    thread->args.args = malloc(size);
    memcpy(thread->args.args, pArgs, size);
}

void free_thread(Thread *thread) {
    #ifdef _WIN32
    CloseHandle(thread->hThread);
    #endif
    free(thread->args.args);
    thread->hThread = NULL;
    thread->args.args = NULL;
    thread->args.size = 0;
}
