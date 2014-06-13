#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include "bitmap.h"

bitmap_t* bmapcreate(int max)
{
	unsigned char *head = NULL;
	bitmap_t *bmap = NULL;
	int cnt;

	for (;;) {
		bmap = (bitmap_t*) calloc(1, sizeof(bitmap_t));
		if (!bmap) break;	

		cnt = max/8 + 1;
		head = (unsigned char *) calloc(cnt, sizeof(uint32_t));
		if (!head) break;

		bmap->head = head;
		bmap->max = max;

		return bmap;
	}

	if (head) free(head);
	if (bmap) free(bmap);

	return NULL;
}

int bmapgetempty(bitmap_t *bmap, int start)
{
	unsigned char *head;
	int block;
	int shift;
	int max;
	int idx = -1;

	if (!bmap) return idx;
	head = bmap->head;
	max = bmap->max;

	block = start / 8;
	shift = start % 8;

	for (; block<max; block++) {
		if (head[block] != 0xff) break;
		shift=0;
	}

	if (block == max) return idx;
	for (; shift<8; shift++) {
		if (!(head[block] & (unsigned char)(0x80 >> shift)))
			break;
	}

	idx = block*8 + shift;

	return idx;
}

int bmapget(bitmap_t *bmap, int idx)
{
	unsigned char *head, old;
	int max;
	int shift;
	int block;

	if (!bmap) return false;

	head = bmap->head;
	max = bmap->max;
	if (idx < 0 || idx >= max) return false;

	block = idx / 8;
	shift = idx % 8;

	old = head[block] & (unsigned char)(0x80 >> shift);

	return old;
}

int bmapset(bitmap_t *bmap, int idx)
{
	unsigned char *head, old;
	int max;
	int shift;
	int block;

	if (!bmap) return false;

	head = bmap->head;
	max = bmap->max;
	if (idx < 0 || idx >= max) return false;

	block = idx / 8;
	shift = idx % 8;

	old = head[block] & (unsigned char)(0x80 >> shift);
	*(head+block) =  head[block] | (unsigned char)(0x80 >> shift);

	return old;
}

int bmapunset(bitmap_t *bmap, int idx)
{
	unsigned char *head, old;
	int max;
	int shift;
	int block;

	if (!bmap) return false;

	head = bmap->head;
	max = bmap->max;
	if (idx < 0 || idx >= max) return false;

	block = idx / 8;
	shift = idx % 8;

	old = head[block] & (unsigned char)(0x80 >> shift);
	*(head+block) =  head[block] & (unsigned char)~(unsigned char)(0x80 >> shift);

	return old;
}

void bmapdel(bitmap_t *bmap)
{
	if (!bmap) return;
	if (bmap->head) free(bmap->head);
	free(bmap);
}