#if defined (_WIN32) || defined (_WIN64)
#include <windows.h>
#include <tchar.h>

#else
#include <semaphore.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "semaphore.h"

sema_t sema_create(int key, int cnt)
{
	sema_t sema;

#if defined (_WIN32) || defined (_WIN64)
	TCHAR strkey[64];
	_stprintf(strkey, _T("Global\\SEMA%u"), key);
	sema = CreateSemaphore( 
				NULL,		// default security attributes
				cnt,		// initial count
				cnt,		// maximum count
				strkey);

	return sema;
#else
	char strkey[64];
	sem_t *sema_;

	for (;;) {
		sprintf(strkey, "SEMA%u", key);
		sema_ = sem_open(strkey, O_CREAT, 0666, cnt);
		if (!sema_) break;

		sema = (sema_t)calloc(1, sizeof(struct semaphore_t));
		if (!sema) break;

		strcpy(sema->name, strkey);
		sema->sema = sema_;

		return sema;
	}

	if (sema_) sem_unlink(strkey);
	if (sema) free(sema);

	return NULL;
#endif
}

int sema_try_enter(sema_t sema, unsigned long millsec)
{
	if (!sema) return 0;

#if defined (_WIN32) || defined (_WIN64)
	if (WaitForSingleObject(sema, millsec) == WAIT_OBJECT_0) {
		return 1;
	}

#else
	const double MILLISECONDS_TO_NANOSECONDS = 1000000.0;
	struct timespec time = {0, 
		millsec * MILLISECONDS_TO_NANOSECONDS
		};

	if (sem_timedwait(sema->sema, &time) == 0) {
		return 1;
	}
#endif

	return 0;
}

int sema_enter(sema_t sema)
{
	if (!sema) return 0;

#if defined (_WIN32) || defined (_WIN64)
	if (WaitForSingleObject(sema, INFINITE) == WAIT_OBJECT_0) {
		return 1;
	}

#else
	if (sem_wait(sema->sema) == 0) {
		return 1;
	}
#endif
	
	return 0;
}

int sema_leave(sema_t sema)
{
	if (!sema) return 0;

#if defined (_WIN32) || defined (_WIN64)
	if (ReleaseSemaphore(sema, 1, NULL)) {
		return 1;
	}
#else
	if (sem_post(sema->sema) == 0) {
		return 1;
	}
#endif

	return 0;
}

void sema_del(sema_t sema)
{
	if (!sema) return;

#if defined (_WIN32) || defined (_WIN64)
	CloseHandle(sema);
#else
	sem_unlink(sema->name);
	free(sema);
#endif
}
