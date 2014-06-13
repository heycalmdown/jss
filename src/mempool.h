#ifndef _MEMPOOL_H
#define _MEMPOOL_H


/**
 mp_t *mp = mempool_create(NULL, 1024, 12);
 void *at1 = mempool_alloc(mp, 1024*1);
 void *at2 = mempool_alloc(mp, 1024*2);
 void *at3 = mempool_alloc(mp, 1024*3);
 mempool_free(mp, at2);
 void *at4 = mempool_alloc(mp, 1024*3);
 mempool_del(mp);
 */

typedef struct mp_hdr_t {
	struct mp_hdr_t *prev;
	struct mp_hdr_t *next;
	int cnt;
	unsigned char data[0];
} mp_hdr_t;

typedef struct mempool_t {
	int size;
	int total;
	int used;
	struct mp_hdr_t *head;
	void *source;
	bool free;
} mp_t;

mp_t* mempool_create(void *source, int size, int cnt);
void* mempool_calloc(mp_t *mp, int cnt);
void* mempool_alloc(mp_t *mp, int size);
void mempool_free(mp_t *mp, void *p);
void mempool_del(mp_t *mp);

#endif