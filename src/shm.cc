#if defined (_WIN32) || defined (_WIN64)
#include <windows.h>
#include <tchar.h>

#else
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h> 
#include <unistd.h>
#include <string.h>
#include <errno.h>
#endif

#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include "error.h"
#include "shm.h"

#if !defined (_WIN32) && !defined (_WIN64)
#define MAX_OPENED_HISTORY	64

static shmid_t _opened_shmids[MAX_OPENED_HISTORY];
static struct sigaction _old_segv_sa;
static struct sigaction _old_int_sa;

static void cleanup(int sig, siginfo_t *si, void *unused)
{
	struct shmid_ds shminfo;
	shmid_t shmid;

	for (int i=0; i < MAX_OPENED_HISTORY; i++) {
		if (_opened_shmids[i] == NULL) 
			continue;

		shmid = _opened_shmids[i];
		shmctl(shmid, IPC_STAT , &shminfo);
		if (shminfo.shm_nattch == 1)
			shmctl(shmid, IPC_RMID, 0);

	}

	if (sig == SIGSEGV && _old_segv_sa.sa_sigaction) {
		_old_segv_sa.sa_sigaction(sig, si, unused);
	} 
	if (sig == SIGINT && _old_int_sa.sa_sigaction) {
		_old_int_sa.sa_sigaction(sig, si, unused);
	}
}

static void set_shm_cleanup() 
{
	static int setted = 0;
	struct sigaction sa;

	if (setted) return;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = cleanup;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, &_old_segv_sa);
	sigaction(SIGINT, &sa, &_old_int_sa);

	setted = 1;
}
#endif

shm_t* shm_create(int key, shm_size_t size, int opt)
{
	shmid_t shmid = 0;
	shm_t *shm = NULL;
	void *at;
	int opt2;

#if defined (_WIN32) || defined (_WIN64)
	TCHAR strkey[64];
	for (;;) {
		if (opt & SHM_READWRITE) opt2 = PAGE_READWRITE;
		else opt2 = PAGE_READONLY;
		_stprintf(strkey, _T("Global\\SHM%u"), key);
		shmid = CreateFileMapping(
			INVALID_HANDLE_VALUE,	// use paging file
			NULL,					// default security
			opt2,	                // read/write access
			DWORD(size >> 32),		// maximum object size (high-order DWORD)
			DWORD(size),			// maximum object size (low-order DWORD)
			strkey);				// name of mapping object
		if (shmid == INVALID_HANDLE_VALUE) {
			printf("CreateFileMapping() failure. (%d)\n", GetLastError());
			break;
		}

		shm = (shm_t*) calloc(1, sizeof(shm_t));
		if (!shm) {
			printf("Can't alloc memory for shm struct. (%d)\n", GetLastError());
			break;
		}

		at = (void*) MapViewOfFile(shmid,   // handle to map object
			opt,							// read/write permission
			0,
			0,
			0);
		if (!at) {
			printf("MapViewOfFile() failure. (%d)\n", GetLastError());
			break;
		}

		shm->shmid = shmid;
		shm->size = size;
		shm->at = at;
		shm->key = key;
		
		return shm;
	}

	if (at) UnmapViewOfFile(at);
	if (shmid) CloseHandle(shmid);
	

#else
	set_shm_cleanup();

	for (;;) {
		shmid = shmget((key_t)key, size, IPC_CREAT|0666);
		if (shmid == -1) break;
		
		shm = (shm_t*) calloc(1, sizeof(shm_t));
		if (!shm) break;

		at = shmat(shmid, (void *) 0, 0);
		if (at == (void *) -1) break;
	
		shm->shmid = shmid;
		shm->size = size;
		shm->at = at;
		shm->key = key;

		/* for cleanup on abnormal termination */
		for (int i=0; i < MAX_OPENED_HISTORY; i++) {
			if (_opened_shmids[i] == NULL) {
				_opened_shmids[i] = shmid;
				break;
			}
		}

		return shm;
	}

	if (shmid) close(shmid);
#endif

	if (shm) free(shm);

	return NULL;
}

void* shm_get(shm_t *shm)
{
	if (!shm) return NULL;
	return shm->at;
}

void shm_del(shm_t *shm)
{
	if (!shm) return;

#if defined (_WIN32) || defined (_WIN64)
	if (shm->at) UnmapViewOfFile(shm->at);
	if (shm->shmid) CloseHandle(shm->shmid);
	
#else
	for (int i=0; i<MAX_OPENED_HISTORY; i++) {
		if (_opened_shmids[i] == shm->shmid) {
			_opened_shmids[i] = NULL;
			break;
		}
	}

	if (shm->at) shmdt(shm->at);
	if (shm->shmid) close(shm->shmid);
#endif

	free (shm);
}