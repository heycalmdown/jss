#ifndef _SHM_H
#define _SHM_H


/**
 shm_t *shm = _shmcreate(key, BUFFER_SIZE, SHM_READWRITE);
 char *at = (char*)_shmget(shm);
 CopyMemory((PVOID)at, szMsg, (_tcslen(szMsg) * sizeof(TCHAR)));
 _shmdel(shm);
 */

#if defined (_WIN32) || defined (_WIN64)
#define SHM_READWRITE	FILE_MAP_WRITE
#define SHM_READONLY	FILE_MAP_READ

typedef HANDLE	shmid_t;
typedef __int64 shm_size_t;

#else
#define SHM_READWRITE	0
#define SHM_READONLY	SHM_RDONLY

typedef int shmid_t;
typedef int shm_size_t;
#endif

typedef struct shared_memory_t {
	unsigned int key;
	void *at;
	shm_size_t size;

	shmid_t shmid;
} shm_t;

shm_t* shm_create(int key, shm_size_t size, int opt);
void* shm_get(shm_t *shm);
void shm_del(shm_t *shm);

#endif