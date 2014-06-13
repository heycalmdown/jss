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
*      $RCSfile: hash.h,v $
*      $Author: johns $        $Locker:  $             $State: Exp $
*      $Revision: 1.15 $      $Date: 2013/09/09 01:42:35 $
*
***************************************************************************
* DESCRIPTION:
*   A simple hash table implementation for strings, contributed by John Stone,
*   derived from his ray tracer code.
***************************************************************************/
#ifndef HASH_H
#define HASH_H


typedef struct hash_userset_t {
   /* Custom allocator support (leave null to use malloc/free) */
   void * (*memalloc) (int cnt, size_t size, void *userdata);
   void (*memfree) (void *, void *userdata);

   void * userdata; /* will be passed to memalloc and memfree */

} hash_userset_t;

typedef struct hash_t {
	int bucketoffset; //struct hash_node_t **bucket;      /* array of hash nodes */
	int size;                         /* size of the array */
	int entries;                      /* number of entries in table */
	int downshift;                    /* shift count, used in hash function */
	int mask;                         /* used to select bits for hashing */

	hash_userset_t us;
} hash_t;

typedef void (*hash_enum_callback_t)(const hash_t *tptr, int idx, char *key, void *data, void *userdata);

#define HASH_FAIL (void*)-1

#if defined(VMDPLUGIN_STATIC)
#define VMDEXTERNSTATIC static
#include "hash.c"
#else

#define VMDEXTERNSTATIC 

#ifdef __cplusplus
extern "C" {
#endif

	hash_t* hash_create(int buckets, hash_userset_t *userset);

	void* hash_lookup (const hash_t *, const char *);

	void hash_enumerator(const hash_t *, hash_enum_callback_t, void *userdata);

	void* hash_insert (hash_t *, const char *, void *);

	void* hash_remove (hash_t *, const char *);

	int hash_entries(hash_t *);

	void hash_destroy(hash_t *);

	char *hash_stats (hash_t *);

#ifdef __cplusplus
}
#endif

#endif

#endif