/***************************************************************************
*
*            (C) Copyright 1995-2011 The Board of Trustees of the
*                        University of Illinois
*                         All Rights Reserved
*
***************************************************************************/

/***************************************************************************
* RCS INFORMATION:
*
*      $RCSfile: hash.c,v $
*      $Author: johns $        $Locker:  $             $State: Exp $
*      $Revision: 1.15 $      $Date: 2013/05/30 21:56:15 $
*
***************************************************************************
* DESCRIPTION:
*   A simple hash table implementation for strings, contributed by John Stone,
*   derived from his ray tracer code.
***************************************************************************/

/*
 * 사용자 메모리 할당자 지원하도록 수정함
 * 내부 자료구조 접근을 OFFSET으로 수정함
 * hash_enumerator() 추가함
 * errprint() 추가함
 * 2014.06 jseol
 */

#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "hash.h"

#define HASH_LIMIT 1

typedef struct hash_node_t {
	int dataoffset;						/* data offset in hash node */
	int keyoffset;						/* key offset for hash lookup */
	int nextoffset;						/* next node offset from hash_t */
} hash_node_t;

static void * memalloc (int cnt, size_t size, void *userdata)
{
	return calloc(cnt, size);
}

static void memfree (void *ptr, void *userdata)
{
	free(ptr);
}

/*
*  hash() - Hash function returns a hash number for a given key.
*
*  tptr: Pointer to a hash table
*  key: The key to create a hash number for
*/
static int hash(const hash_t *tptr, const char *key) {
	int i=0;
	int hashvalue;

	while (*key != '\0')
		i=(i<<3)+(*key++ - '0');

	hashvalue = (((i*1103515249)>>tptr->downshift) & tptr->mask);
	if (hashvalue < 0) {
		hashvalue = 0;
	}    

	return hashvalue;
}

/*
*  hash_init() - Initialize a new hash table.
*
*  tptr: Pointer to the hash table to initialize
*  buckets: The number of initial buckets to create
*/
static int hash_init(hash_t *tptr, int buckets) {
	void *nodearray;

	/* make sure we allocate something */
	if (buckets==0)
		buckets=16;

	/* initialize the table */
	tptr->entries=0;
	tptr->size=2;
	tptr->mask=1;
	tptr->downshift=29;

	/* ensure buckets is a power of 2 */
	while (tptr->size<buckets) {
		tptr->size<<=1;
		tptr->mask=(tptr->mask<<1)+1;
		tptr->downshift--;
	} /* while */

	/* allocate memory for table */
	nodearray = tptr->us.memalloc(tptr->size, sizeof(int), tptr->us.userdata);
	if (!nodearray) return false;
	tptr->bucketoffset = (unsigned char *) tptr - (unsigned char *) nodearray;
	errprint("[hash_init] nodearray(0x%x), offset(%d)", nodearray, tptr->bucketoffset);
	return true;
}	

VMDEXTERNSTATIC hash_t* hash_create(int buckets, hash_userset_t *userset) {
	hash_userset_t us, *usptr = userset;
	hash_t *tptr;

	if (!usptr) {
		usptr = &us;
		us.memalloc = memalloc;
		us.memfree = memfree;
		us.userdata = NULL;
	}

	tptr = (hash_t *) usptr->memalloc(1, sizeof(hash_t), usptr->userdata);
	if (!tptr) return NULL;
	memcpy(&tptr->us, usptr, sizeof(hash_userset_t));

	if (!hash_init(tptr, buckets)) {
		hash_destroy(tptr);
		return NULL;
	}
	
	errscope(
		int *nodearray = (int *) ((unsigned char*) tptr - tptr->bucketoffset);
		errprint("tptr(0x%x), bucketoffset(0x%x, %d), count(%d/%d)", tptr, nodearray, tptr->bucketoffset, buckets, tptr->size);
		for(int i=0; i<tptr->size; i++) {
			errprint("bucket(%d) is %d", i, nodearray[i]);
		}
	);

	return tptr;
}

/*
*  rebuild_table() - Create new hash table when old one fills up.
*
*  tptr: Pointer to a hash table
*/
static int rebuild_table(hash_t *tptr) {
	hash_node_t *old_hash;
	int old_size, h, i, *new_bucket, *old_bucket, offset;

	old_bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);
	old_size=tptr->size;

	/* create a new table and rehash old buckets */
	if (!hash_init(tptr, old_size<<1))
		return false;
	new_bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);
	for (i=0; i<old_size; i++) {
		for (offset = old_bucket[i]; offset != 0; ) {
			old_hash=(hash_node_t *) ((unsigned char*) tptr - offset);
			offset = old_hash->nextoffset;

			h=hash(tptr, (char *)((unsigned char *) tptr - old_hash->keyoffset));
			old_hash->nextoffset=new_bucket[h];
			new_bucket[h]=((unsigned char*) tptr - (unsigned char *) old_hash);
			tptr->entries++;
		}
	} /* for */

	/* free memory used by old table */
	tptr->us.memfree(old_bucket, tptr->us.userdata);

	return true;
}

/*
*  hash_lookup() - Lookup an entry in the hash table and return a pointer to
*    it or HASH_FAIL if it wasn't found.
*
*  tptr: Pointer to the hash table
*  key: The key to lookup
*/
VMDEXTERNSTATIC void* hash_lookup(const hash_t *tptr, const char *key) {
	int h, *bucket, offset;
	hash_node_t *node = NULL;

	bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);

	errscope(
		errprint("tptr(0x%x), tptr->bucketoffset(%d)", tptr, tptr->bucketoffset);
		for(int i=0; i<tptr->size; i++) {
			errprint("bucket(%d) is %d", i, bucket[i]);
		}
	);

	/* find the entry in the hash table */
	h=hash(tptr, key);
	for (offset = bucket[h]; offset != 0; offset = node->nextoffset) {
		node=(hash_node_t *)((unsigned char *) tptr - offset);
		errprint("tptr(0x%x), bucket(%d,%d), offset(%d), node(0x%x), key(%s)", tptr, h, bucket[h], offset,node,key);
		if (!strcmp((char *)((unsigned char *) tptr - node->keyoffset), key)) {
			break;
		}
	}

	/* return the entry if it exists, or HASH_FAIL */
	return(offset ? (void*)((unsigned char *) tptr - node->dataoffset) : HASH_FAIL);
}

VMDEXTERNSTATIC void hash_enumerator(const hash_t *tptr, hash_enum_callback_t callee, void *userdata) {
	hash_node_t *node;
	int i, c=0, *bucket, offset;
	char *key;
	void *data;

	bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);

	if (tptr->bucketoffset) {
		for (i=0; i<tptr->size; i++) {
			for (offset = bucket[i]; offset != 0; offset = node->nextoffset) {
				node = (hash_node_t *)((unsigned char *) tptr - offset);
				key = (char *)((unsigned char *) tptr - node->keyoffset);
				data = (void *)((unsigned char *) tptr - node->dataoffset);
				callee(tptr, c++, key, data, userdata);
			}
		} 
	}
}

/*
*  hash_insert() - Insert an entry into the hash table.  If the entry already
*  exists return a pointer to it, otherwise return HASH_FAIL.
*
*  tptr: A pointer to the hash table
*  key: The key to insert into the hash table
*  data: A pointer to the data to insert into the hash table
*/
VMDEXTERNSTATIC void* hash_insert(hash_t *tptr, const char *key, void *data) {
	void *tmp;
	hash_node_t *node;
	int h, *bucket;

	bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);

	/* check to see if the entry exists */
	if ((tmp=hash_lookup(tptr, key)) != HASH_FAIL)
		return(tmp);
	/* expand the table if needed */
	while (tptr->entries>=HASH_LIMIT*tptr->size) {
		if (!rebuild_table(tptr))
			return HASH_FAIL;
	}

	bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);

	/* insert the new entry */
	h=hash(tptr, key);
	node=(struct hash_node_t *) tptr->us.memalloc(1, sizeof(hash_node_t), tptr->us.userdata);
	if (!node) return HASH_FAIL;
	node->dataoffset=(unsigned char *) tptr - (unsigned char *) data;
	node->keyoffset=(unsigned char *) tptr - (unsigned char *) key;
	node->nextoffset=bucket[h];
	bucket[h]= (unsigned char *) tptr - (unsigned char *) node;
	tptr->entries++;
	errprint("tptr(0x%x), key(%s), bucket(%d, %d), node(0x%x), node.nextoffset(%d)", tptr, key, h, bucket[h], node, node->nextoffset);

	return node;
}

/*
*  hash_remove() - Remove an entry from a hash table and return a pointer
*  to its data or HASH_FAIL if it wasn't found.
*
*  tptr: A pointer to the hash table
*  key: The key to remove from the hash table
*/
VMDEXTERNSTATIC void* hash_remove(hash_t *tptr, const char *key) {
	hash_node_t *node = NULL, *last = NULL;
	void *data;
	int h, *bucket, offset;

	bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);

	/* find the node to remove */
	h=hash(tptr, key);
	for (offset = bucket[h]; offset != 0; offset = node->nextoffset) {
		node=(hash_node_t *)((unsigned char *) tptr - offset);
		if (strcmp((char *)((unsigned char *) tptr - node->keyoffset), key)) {
			last = node;
			continue;
		}

		if (offset == bucket[h]) {
		/* if node is at head of bucket, we have it easy */
			bucket[h]=node->nextoffset;
		} else {
		/* find the node before the node we want to remove */
			last->nextoffset = node->nextoffset;
		}
		break;
	}

	/* Didn't find anything, return HASH_FAIL */
	if (offset == 0)
		return HASH_FAIL;

	/* free memory and return the data */
	data=(void*)((unsigned char *) tptr - node->dataoffset);
	tptr->us.memfree(node, tptr->us.userdata);
	tptr->entries--;

	return data;
}


/*
* inthash_entries() - return the number of hash table entries.
*
*/
VMDEXTERNSTATIC int hash_entries(hash_t *tptr) {
	return tptr->entries;
}


/*
* hash_destroy() - Delete the entire table, and all remaining entries.
* 
*/
VMDEXTERNSTATIC void hash_destroy(hash_t *tptr) {
	hash_node_t *node;
	int i, *bucket, offset;

	bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);
	
	if (tptr->bucketoffset) {
		for (i=0; i<tptr->size; i++) {
			for (offset = bucket[i]; offset != 0; offset = node->nextoffset) {
				node=(hash_node_t *)((unsigned char *) tptr - offset);
				tptr->us.memfree(node, tptr->us.userdata);
			}
		}   

		/* free the entire array of buckets */
		tptr->us.memfree(bucket, tptr->us.userdata);
	}

	tptr->us.memfree(tptr, tptr->us.userdata);
}

/*
*  alos() - Find the average length of search.
*
*  tptr: Pointer to a hash table
*/
static float alos(hash_t *tptr) {
	int i,j, *bucket, offset;
	float alos=0;
	hash_node_t *node;

	bucket = (int *) ((unsigned char*) tptr - tptr->bucketoffset);

	for (i=0; i<tptr->size; i++) {
		for (offset = bucket[i], j=0; offset != 0; offset = node->nextoffset, j++);
		if (j)
			alos+=((j*(j+1))>>1);
	} /* for */

	return(tptr->entries ? alos/tptr->entries : 0);
}

/*
*  hash_stats() - Return a string with stats about a hash table.
*
*  tptr: A pointer to the hash table
*/
VMDEXTERNSTATIC char * hash_stats(hash_t *tptr) {
	static char buf[1024];

	sprintf(buf, "%u slots, %u entries, and %1.2f ALOS",
		(int)tptr->size, (int)tptr->entries, alos(tptr));

	return(buf);
}