#ifndef __THREAD_H__
#define __THREAD_H__

#ifdef _WIN32
#include <windows.h>
#include <process.h>

#define thread_exit(x) _endthreadex(x)
#endif

#define THREAD_OK               0
#define THREAD_NO_RUN_CALLBACK -1

typedef unsigned int (*ThreadRunCallback)(void *pParam);
typedef void (*ThreadEndCallback)();


// Contient les données utilisées par le thread pendant son exécution.
typedef struct ThreadArgs {
    void *args;
    unsigned long long size;
} ThreadArgs;

typedef struct Thread {
    #ifdef _WIN32
    HANDLE hThread;
    #endif
    ThreadRunCallback runCallback;
    ThreadEndCallback endCallback;
    ThreadArgs args;
} Thread;

// Créer un thread.
void create_thread(Thread *thread, ThreadRunCallback runCallback, ThreadEndCallback endCallback);

// Démarre le thread.
char thread_run(Thread *thread);

// Attend la fin d'exécution d'un thread.
void thread_wait(Thread *thread);

/*
        Copie les données pointées par pArgs dans la structure
        pour une utilisation interne plus simple.      
*/
void thread_set_args(Thread *thread, void *pArgs, unsigned long long size);

void free_thread(Thread *thread);

#endif