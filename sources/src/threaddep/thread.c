 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Threading support, using pthreads
  *
  * Copyright 1997 Bernd Schmidt
  * Copyright 2004 Richard Drummond
  *
  * This handles initialization when using named semaphores.
  * Idea stolen from SDL.
  */
#ifdef WIN32PORT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "thread.h"
void uae_sem_init (uae_sem_t * event, int manual_reset, int initial_state)
{
        if(*event) {
                if (initial_state)
                        SetEvent (*event);
                else
                        ResetEvent (*event);
        } else {
                *event = CreateEvent (NULL, manual_reset, initial_state, NULL);
        }
}

void uae_sem_wait (uae_sem_t * event)
{
        WaitForSingleObject (*event, INFINITE);
}

void uae_sem_post (uae_sem_t * event)
{
        SetEvent (*event);
}

int uae_sem_trywait (uae_sem_t * event)
{
        return WaitForSingleObject (*event, 0) == WAIT_OBJECT_0 ? 0 : -1;
}

void uae_sem_destroy (uae_sem_t * event)
{
        if (*event) {
                CloseHandle (*event);
                *event = NULL;
        }
}



typedef unsigned (__stdcall *BEGINTHREADEX_FUNCPTR)(void *);

struct thparms
{
        void *(*f)(void*);
        void *arg;
};

static unsigned __stdcall thread_init (void *f)
{
        struct thparms *thp = (struct thparms*)f;
        void *(*fp)(void*) = thp->f;
        void *arg = thp->arg;

        xfree (f);


                fp (arg);



        return 0;
}

void uae_end_thread (uae_thread_id *tid)
{
        if (tid) {
                CloseHandle (*tid);
                *tid = NULL;
        }
}

int uae_start_thread (const TCHAR *name, void *(*f)(void *), void *arg, uae_thread_id *tid)
{
        HANDLE hThread;
        int result = 1;
        unsigned foo;
        struct thparms *thp;

        thp = xmalloc (struct thparms, 1);
        thp->f = f;
        thp->arg = arg;
        hThread = (HANDLE)_beginthreadex (NULL, 0, thread_init, thp, 0, &foo);
        if (hThread) {
                if (name) {
                        //write_log (_T("Thread '%s' started (%d)\n"), name, hThread);
                        
                                SetThreadPriority (hThread, THREAD_PRIORITY_HIGHEST);
                        
                }
        } else {
                result = 0;
                write_log (_T("Thread '%s' failed to start!?\n"), name ? name : _T("<unknown>"));
        }
        if (tid)
                *tid = hThread;
        else
                CloseHandle (hThread);
        return result;
}

int uae_start_thread_fast (void *(*f)(void *), void *arg, uae_thread_id *tid)
{
        int v = uae_start_thread (NULL, f, arg, tid);
        if (*tid) {
                 SetThreadPriority (*tid, THREAD_PRIORITY_HIGHEST);
                
        }
        return v;
}

DWORD_PTR cpu_affinity = 1, cpu_paffinity = 1;

void uae_set_thread_priority (uae_thread_id *tid/*, int pri*/)
{
#if 0
        int pri2;
        HANDLE th;

        if (tid)
                th = *tid;
        else
                th = GetCurrentThread ();
        pri2 = GetThreadPriority (th);
        if (pri2 == THREAD_PRIORITY_ERROR_RETURN)
                pri2 = 0;
        if (pri > 0)
                pri2 = THREAD_PRIORITY_HIGHEST;
        else
                pri2 = THREAD_PRIORITY_ABOVE_NORMAL;
        pri2 += pri;
        if (pri2 > 1)
                pri2 = 1;
        if (pri2 < -1)
                pri2 = -1;
        SetThreadPriority (th, pri2);
#endif

                if (!SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
                        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

}

#else

#include "sysconfig.h"
#include "sysdeps.h"

#include <pthread.h>
#include <semaphore.h>

#include "thread.h"

#ifdef USE_NAMED_SEMAPHORES

int uae_sem_init (uae_sem_t *sem, int pshared, unsigned int value)
{
    char name[32];
    static int semno = 0;
    int result = 0;

    sprintf (name, "/uaesem-%d-%d", getpid (), semno++);

    if ((sem->sem = sem_open (name, O_CREAT, 0600, value)) != (sem_t *)SEM_FAILED)
		sem_unlink (name);
    else {
		sem->sem = 0;
		result = -1;
    }
    return result;
}

#else

int uae_sem_init (uae_sem_t *sem, int pshared, unsigned int value)
{
	if (!sem || (sem && sem->sem))
		return -1;
	sem->sem = (sem_t*)calloc(0, sizeof(sem_t));
	return sem_init (sem->sem, pshared, value);
}

#endif

#endif
