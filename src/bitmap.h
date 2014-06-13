#ifndef _BITMAP_H
#define _BITMAP_H

/**
 bitmap_t *bmap = bmapcreate(1024);
 bool ret;
 int pos;
 pos = bmapgetempty(bmap, 0);
 ret = bmapset(bmap, 1);
 pos = bmapgetempty(bmap, 0);
 pos = bmapgetempty(bmap, 1);
 ret = bmapset(bmap, 0);
 pos = bmapgetempty(bmap, 0);
 ret = bmapset(bmap, 8);
 ret = bmapset(bmap, 9);
 ret = bmapunset(bmap, 8);
 ret = bmapunset(bmap, 9);
 bmapdel(bmap);
 */

typedef struct _bitmap {
	unsigned char *head;
	int max;
} bitmap_t;

bitmap_t* bmapcreate(int max);
int bmapgetempty(bitmap_t *bmap, int start);
int bmapget(bitmap_t *bmap, int idx);
int bmapset(bitmap_t *bmap, int idx);
int bmapunset(bitmap_t *bmap, int idx);
void bmapdel(bitmap_t *bmap);

#endif