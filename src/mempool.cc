#include <malloc.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "error.h"
#include "mempool.h"

mp_t* mempool_create(void *source, int size, int cnt)
{
	void *source_ = source;
	mp_t *mp = NULL;
	int real = size + sizeof(mp_hdr_t);
	int totaol = 0;
	mp_hdr_t *mphdr;
	
	if (!size || !cnt) 
		return NULL;

	for (;;) {
		mp = (mp_t*) calloc(1, sizeof(mp_t));
		if (!mp) break;

		if (!source_) 
			source_ = calloc(1, real*cnt);
		if (!source_) break;

		mp->head = mphdr = (mp_hdr_t *) source_;
		int i;
		
		try {
			for (i=0; i<cnt-1; i++) {

				mphdr->next = (mp_hdr_t *) ((unsigned char *) mphdr + real);
				mphdr->prev = (mp_hdr_t *) ((unsigned char *) mphdr - real);
				errscope(
					if (i < 5 || i > cnt-5)
						errprint("[%d] mphdr=0x%x, mphdr.prev=0x%x, mphdr.next=0x%x", i, mphdr, mphdr->prev, mphdr->next);
				);
				mphdr = mphdr->next;

			}

			mphdr->prev = (mp_hdr_t *) ((unsigned char *) mphdr - real);
			mphdr->next = mp->head;
		} catch (...) {
			break;
		}

		mp->head->prev = mphdr;
		mp->source = source_;
		mp->total = cnt;
		mp->size = size;
		if (!source) mp->free = true;

		errscope(
			int c=mp->total-mp->used;
			mp_hdr_t *it=mp->head;
			for(int j=0; j<c; it=it->next, j++) {
				errprint("[%d] mphdr=0x%x, mphdr.prev=0x%x, mphdr.next=0x%x", j, it, it->prev, it->next);
			}
		);

		return mp;
	}

	if (!source && source_) free(source_);
	if (mp) free(mp);

	return NULL;
}

void* mempool_calloc(mp_t *mp, int cnt)
{
	mp_hdr_t *mphdr, *start, *end;
	int total;
	int size;
	int used;
	int real;

	if (!mp || !cnt) return NULL;
	total = mp->total;
	used = mp->used;
	mphdr = mp->head;
	real = mp->size + sizeof(mp_hdr_t);

	/* cnt개의 연속하는 구간이 존재하는지 검사 */
	int x,y,c = total - used;
	start = end = mphdr;
	for (x=0, y=1; x < c; x++) {
		if (y == cnt) break;

		if ((unsigned char *) mphdr->next - (unsigned char *) mphdr == real) {
			y++;
		} else {
			y=0;
		}

		if (y == 1) start = mphdr;
		mphdr = mphdr->next;
		if (y) end = mphdr;		
	}
	if (x == c) return NULL;

	mp->head = end->next;
	(end->next)->prev = start->prev;
	(start->prev)->next = end->next;

	start->cnt = cnt;
	mp->used += cnt;

	errscope(
		errprint("start=0x%x, count=%d", start, cnt);
		int cc=mp->total-mp->used;
		mp_hdr_t *it=mp->head;
		for(int j=0; j<cc; it=it->next, j++) {
			errprint("[%d] mphdr=0x%x, mphdr.prev=0x%x, mphdr.next=0x%x", j, it, it->prev, it->next);
		}
	);

	return start->data;
}

void* mempool_alloc(mp_t *mp, int size)
{
	int real;
	int cnt;

	if (!mp || !size) 
		return NULL;

	real = size + sizeof(mp_hdr_t);
	cnt = real / mp->size;
	if (real % mp->size) cnt++;

	return mempool_calloc(mp, cnt);
}

void mempool_free(mp_t *mp, void *p)
{
	mp_hdr_t *mphdr, *start, *end, *iter;
	int cnt, i;
	int real;

	if (!mp || !p) return;

	real = mp->size + sizeof(mp_hdr_t);

	mphdr = (mp_hdr_t *) ((unsigned char*)p - sizeof(mp_hdr_t));
	start = mphdr;	
	cnt = start->cnt;
	end = (mp_hdr_t *) ((unsigned char *) mphdr + real*(cnt-1));

	for (i=0; i<cnt; i++) {
		mphdr =  (mp_hdr_t *) ((unsigned char *) mphdr + real*i);
		if (i == 0) mphdr->prev = NULL;
		else mphdr->prev = (mp_hdr_t *) ((unsigned char *) mphdr - real);

		if (i+1 == cnt) mphdr->next = NULL;
		else mphdr->next = (mp_hdr_t *) ((unsigned char *) mphdr + real);
	}

	if (mp->total - mp->used == 0) {
		mp->head = start;
		start->prev = end;
		end->next = start;

	} else {
		for (iter=mp->head, i=0; i<mp->total-mp->used; iter=iter->next, i++) {
			if (start < iter) break;			
		}

		(iter->prev)->next = end;
		start->prev = iter->prev;
		end->next = iter;

		iter->prev = end;
	}

	if (mp->head > start) {
		mp->head = start;
	}

	mp->used -= cnt;

	errscope(
		errprint("start=0x%x, end=0x%x, cnt=%d", start, end, cnt);

		int cc=mp->total-mp->used;
		mp_hdr_t *it=mp->head;
		for(int j=0; j<cc; it=it->next, j++) {
			errprint("[%d] mphdr=0x%x, mphdr.prev=0x%x, mphdr.next=0x%x", j, it, it->prev, it->next);
		}
	);
}

void mempool_del(mp_t *mp)
{
	if (!mp) return;
	if (mp->free) free(mp->source);
	free(mp);
}