
/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 1999-2004, Applera Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received (LICENSE.txt) a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *************************************************************************/
static char *rcsid = "$Id: AS_UTL_Hash.c,v 1.28 2012-05-08 23:17:55 brianwalenz Exp $";

#include "AS_UTL_Hash.h"
#include "AS_UTL_fileIO.h"

//  Debugging targets

//  When doing an insert, check that the key doesn't exist immediately before inserting.
//
#define CHECKCOLLISION

//  Hash table parameters.  We'll build a bigger table when we've loaded the maximum number of nodes
//  allowed (maxNodes).  This value is determined from the number of hash buckets currently
//  allocated (numBuckets * LOAD_FACTOR).
//
#define LOAD_FACTOR         0.6

//  mix -- mix 3 32-bit values reversibly.
//
//  For every delta with one or two bits set, and the deltas of all
//  three high bits or all three low bits, whether the original value
//  of a,b,c is almost all zero or is uniformly distributed,
//
//  * If mix() is run forward or backward, at least 32 bits in a,b,c
//    have at least 1/4 probability of changing.
//
//  * If mix() is run forward, every bit of c will change between 1/3
//    and 2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit
//    deltas.)
//
//  mix() takes 36 machine instructions, but only 18 cycles on a
//  superscalar machine (like a Pentium or a Sparc).  No faster mixer
//  seems to work, that's the result of my brute-force search.  There
//  were about 2^68 hashes to choose from.  I only tested about a
//  billion of those.
//
#define mix(a,b,c) \
  {                  \
    a -= b;          \
    a -= c;          \
    a ^= (c>>13);    \
    b -= c;          \
    b -= a;          \
    b ^= (a<<8);     \
    c -= a;          \
    c -= b;          \
    c ^= (b>>13);    \
    a -= b;          \
    a -= c;          \
    a ^= (c>>12);    \
    b -= c;          \
    b -= a;          \
    b ^= (a<<16);    \
    c -= a;          \
    c -= b;          \
    c ^= (b>>5);     \
    a -= b;          \
    a -= c;          \
    a ^= (c>>3);     \
    b -= c;          \
    b -= a;          \
    b ^= (a<<10);    \
    c -= a;          \
    c -= b;          \
    c ^= (b>>15);    \
  }

//  Hash_AS -- hash a variable-length key into a 32-bit value
//
//    k       : the key (the unaligned variable-length array of bytes)
//    len     : the length of the key, counting by bytes
//    initval : can be any 4-byte value
//
//  Returns a 32-bit value.  Every bit of the key affects every bit of
//  the return value.  Every 1-bit and 2-bit delta achieves avalanche.
//  About 6*len+35 instructions.
//
//  The best hash table sizes are powers of 2.  There is no need to do
//  mod a prime (mod is sooo slow!).  If you need less than 32 bits,
//  use a bitmask.  For example, if you need only 10 bits, do
//
//    h = (h & hashmask(10));
//
//  In which case, the hash table should have hashsize(10) elements.
//
//  If you are hashing n strings (uint8 **)k, do it like this:
//    for (i=0, h=0; i<n; ++i) h = Hash_AS k[i], len[i], h);
//
//  By Bob Jenkins, 1996.  bob_jenkins@compuserve.com.  You may use
//  this code any way you wish, private, educational, or commercial.
//  It's free.  See http://ourworld.compuserve.com/homepages/bob_jenkins/evahash.htm
//  Use for hash table lookup, or anything where one collision in
//  2^^32 is acceptable.  Do NOT use for cryptographic purposes.
//
uint32 Hash_AS(uint8  *k,       // the key
               uint32  length,  // the length of the key
               uint32  initval) // the previous hash, or an arbitrary value
{
   register uint32 a,b,c,len;

   /* Set up the internal state */
   len = length;
   a = b = 0x9e3779b9U;  /* the golden ratio; an arbitrary value */
   c = initval;         /* the previous hash value */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a += (k[0] +((uint32)k[1]<<8) +((uint32)k[2]<<16) +((uint32)k[3]<<24));
      b += (k[4] +((uint32)k[5]<<8) +((uint32)k[6]<<16) +((uint32)k[7]<<24));
      c += (k[8] +((uint32)k[9]<<8) +((uint32)k[10]<<16)+((uint32)k[11]<<24));
      mix(a,b,c);
      k += 12;
      len -= 12;
   }
   /*------------------------------------- handle the last 11 bytes */
   c += length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((uint32)k[10]<<24);
   case 10: c+=((uint32)k[9]<<16);
   case 9 : c+=((uint32)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((uint32)k[7]<<24);
   case 7 : b+=((uint32)k[6]<<16);
   case 6 : b+=((uint32)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((uint32)k[3]<<24);
   case 3 : a+=((uint32)k[2]<<16);
   case 2 : a+=((uint32)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}








int
InsertNodeInHashBucket(HashTable_AS *table,
                       HashNode_AS *newnode) {

  uint32       hashkey = (*table->hash)(newnode->key, newnode->keyLength);
  uint32       bucket  = hashkey & table->hashmask;
  HashNode_AS *node    = table->buckets[bucket];
  HashNode_AS *prevnode;
  int          comparison;

  //  We never have a next, yet.
  newnode->next = NULL;

  //  If bucket is empty, insert at head of bucket.  Should usually be
  //  this way!
  //
  if (node == NULL) {
    table->buckets[bucket] = newnode;
    table->dirty = 1;
    return HASH_SUCCESS;
  }

  comparison = (*table->compare)(node->key, newnode->key);

  //  element already present?
  if (comparison == 0)
    return(HASH_FAILURE);

  //  Insert at head of bucket?
  if (comparison < 0) {
#ifdef CHECKCOLLISION
    if (LookupInHashTable_AS(table, newnode->key, newnode->keyLength, NULL, NULL)) {
      fprintf(stderr, "KEY EXISTS!  (1)\n");
      assert(0);
    }
#endif
    newnode->next          = table->buckets[bucket];
    table->buckets[bucket] = newnode;
    table->dirty = 1;
    return HASH_SUCCESS;
  }

  prevnode = node;
  node     = node->next;

  //  Insert in the list?
  while (node) {
    comparison = (*table->compare)(node->key, newnode->key);

    if (comparison == 0)
      return(HASH_FAILURE);

    if (comparison < 0) {
#ifdef CHECKCOLLISION
      if (LookupInHashTable_AS(table, newnode->key, newnode->keyLength, NULL, NULL)) {
        fprintf(stderr, "KEY EXISTS!  (1)\n");
        assert(0);
      }
#endif
      newnode->next  = node;
      prevnode->next = newnode;
      table->dirty = 1;
      return HASH_SUCCESS;
    }

    prevnode = node;
    node     = node->next;
  }

#ifdef CHECKCOLLISION
  if (LookupInHashTable_AS(table, newnode->key, newnode->keyLength, NULL, NULL)) {
    fprintf(stderr, "KEY EXISTS!  (1)\n");
    assert(0);
  }
#endif

  //  Append to the end of the list
  prevnode->next = newnode;
  table->dirty = 1;

  return HASH_SUCCESS;
}




//  Increase the size to the next power of two
void
ReallocHashTable_AS(HashTable_AS *htable) {
  HashNode_AS     *node;
  HeapIterator_AS  iterator;

  //  To ensure things stay powers of two, we use shift and not multiply.

  uint64 newNumBuckets = htable->numBuckets << 1;
  uint64 newMaxNodes   = htable->numNodes   << 1;

#if 0
  fprintf(stderr, "ReallocHashTable_AS()-- from "F_U64" to "F_U64" buckets (max nodes: "F_U64" to "F_U64")%s.\n",
          htable->numBuckets, newNumBuckets,
          htable->maxNodes, newMaxNodes,
          (newNumBuckets > htable->maxBuckets) ? ": TOO LARGE, DON'T EXPAND." : "");
#endif

  //  Too many buckets?  Don't rebuild the table, but allow more nodes.
  if (newNumBuckets > htable->maxBuckets) {
    htable->maxNodes = newMaxNodes;
    return;
  }

  htable->freeList      = NULL;
  htable->numBuckets    = newNumBuckets;
  htable->maxNodes      = newMaxNodes;
  htable->hashmask    <<= 1;
  htable->hashmask     |= 1;

  safe_free(htable->buckets);
  htable->buckets = (HashNode_AS **)safe_calloc(htable->numBuckets, sizeof(HashNode_AS *));

  InitHeapIterator_AS(htable->nodeheap, &iterator);
  while (NULL != (node = (HashNode_AS *)NextHeapIterator_AS(&iterator)))
    if (node->isFree == 0)
      if (InsertNodeInHashBucket(htable, node) == HASH_FAILURE) {
        fprintf(stderr, "ReallocHashTable_AS()-- Failed to insert node; duplicate?\n");
        assert(0);
      }
}



HashTable_AS *
CreateGenericHashTable_AS(ASHashHashFn  hashfn,
                          ASHashCompFn  compfn) {
  uint32        size  = 4096;  //  MUST be power of two, otherwise the hashed value breaks.
  HashTable_AS *table = (HashTable_AS *)safe_calloc(1, sizeof(HashTable_AS));

  //  Why are we no longer requesting the user to size the table?  For the tables that are large --
  //  the number of fragments -- we usually do not know the number in advance.  We used to just
  //  guess at 1m.  Reallocations up to there are pretty cheap, so always defaulting to a size of 4k
  //  doesn't cost us much.
  //
  //  N.B. 'size=4096' is hardcoded, as is the corresponding 'hashmask' below.
  //  N.B. 'maxBuckets' is hardcoded; this is an algorithmic limit of the hash table.
  //
  //  We could probably go to maxBuckets of 1<<32, but why chance it.

  table->numBuckets  = size;
  table->maxBuckets  = (uint64)1 << 31;
  table->buckets     = (HashNode_AS **)safe_calloc(table->numBuckets, sizeof(HashNode_AS *));
  table->freeList    = NULL;
  table->numNodes    = 0;
  table->maxNodes    = (uint64)(size * LOAD_FACTOR);
  table->nodeheap    = AllocateHeap_AS(sizeof(HashNode_AS));
  table->hashmask    = 0x00000fff;  //  Keyed specifically to 'size' above.
  table->dirty       = 0;
  table->filename[0] = 0;
  table->compare     = compfn;
  table->hash        = hashfn;

  assert(table->hashmask == table->numBuckets - 1);

  return(table);
}



static
int
INThashfunction(uint64 k, uint32 l) {
  assert((l == 0) || (l == 8));
  return(Hash_AS((uint8 *)&k, sizeof(uint64), 37));
}
static
int
INTcomparefunction(uint64 ka, uint64 kb) {
  if (ka < kb)
    return(-1);
  if (ka > kb)
    return(1);
  return(0);
}
HashTable_AS *
CreateScalarHashTable_AS(void) {
  return(CreateGenericHashTable_AS(INThashfunction, INTcomparefunction));
}



static
int
STRhashfunction(uint64 k, uint32 l) {
  //  k is a pointer to the string
  return(Hash_AS((uint8 *)(INTPTR)k, l, 37));
}
static
int
STRcomparefunction(uint64 ka, uint64 kb) {
  //  ka, kb are pointers to the strings
  char *kas = (char *)(INTPTR)ka;
  char *kbs = (char *)(INTPTR)kb;
  return(strcmp(kas, kbs));
}
HashTable_AS *
CreateStringHashTable_AS(void) {
  return(CreateGenericHashTable_AS(STRhashfunction, STRcomparefunction));
}


void
ResetHashTable_AS(HashTable_AS *table) {
  memset(table->buckets, 0, table->numBuckets * sizeof(HashNode_AS *));
  table->freeList   = NULL;
  table->numNodes   = 0;
  table->dirty      = 1;
  FreeHeap_AS(table->nodeheap);
  table->nodeheap  = AllocateHeap_AS(sizeof(HashNode_AS));
}


void
DeleteHashTable_AS(HashTable_AS *table) {

  if (table == NULL)
    return;

  if (table->dirty && table->filename[0])
    SaveHashTable_AS(table->filename, table);

  safe_free(table->buckets);
  FreeHeap_AS(table->nodeheap);
  safe_free(table);
}




//  Insert a new key into the hash table.  Returns SUCCESS if the key
//  doesn't already exist.
int
InsertInHashTable_AS(HashTable_AS  *table,
                     uint64         key,
                     uint32         keylen,
                     uint64         value,
                     uint32         valuetype) {

  HashNode_AS *node;

  if (table->freeList) {
    node = table->freeList;
    table->freeList = node->next;
  } else {
    if (table->numNodes >= table->maxNodes)
      ReallocHashTable_AS(table);
    node = (HashNode_AS *)GetHeapItem_AS(table->nodeheap);
  }

  node->key          = key;
  node->value        = value;
  node->isFree       = 0;
  node->keyLength    = keylen;
  node->valueType    = valuetype;
  node->next         = NULL;

  if (InsertNodeInHashBucket(table, node) == HASH_SUCCESS) {
    table->numNodes++;
    return(HASH_SUCCESS);
  }

  //  Otherwise, the key is already there.  Return this node to the free list.

  node->key          = 0;
  node->value        = 0;
  node->keyLength    = 0;
  node->valueType    = 0;
  node->isFree       = 1;
  node->next         = table->freeList;

  table->freeList = node;

  return(HASH_FAILURE);
}



int
DeleteFromHashTable_AS(HashTable_AS *table,
                       uint64        key,
                       uint32        keylen) {

  int32        hashkey   = (*table->hash)(key, keylen);
  int          bucket    = hashkey & table->hashmask;
  HashNode_AS *node      = NULL;
  HashNode_AS *prev      = NULL;

  assert(bucket >= 0);
  assert(bucket <= table->numBuckets);

#if 0
  if (keylen > 0)
    fprintf(stderr, "delete - key "F_U64" %d,%d,%d keylen %d hashkey %d bucket %d\n",
            key,
            ((int *)key)[0],
            ((int *)key)[1],
            ((int *)key)[2],
            keylen,
            hashkey,
            bucket);
#endif

  node = table->buckets[bucket];
  while (node) {
    int comparison = (*table->compare)(node->key, key);

    if (comparison == 0) {
      if (prev)
	prev->next             = node->next;
      else
	table->buckets[bucket] = node->next;

      node->key          = 0;
      node->value        = 0;
      node->keyLength    = 0;
      node->valueType    = 0;
      node->isFree       = 1;
      node->next         = table->freeList;

      table->freeList = node;
      table->numNodes--;

      table->dirty = 1;

      return HASH_SUCCESS;
    }

    if (comparison < 0)
      //  The nodes are ordered by key, and we're past where it should be.
      return HASH_FAILURE;

    prev = node;
    node = node->next;
  }

  return HASH_FAILURE;
}



//  Replace the value of a key, or add a new key.  Returns SUCCESS if
//  the key was updated or added.
uint64
ReplaceInHashTable_AS(HashTable_AS  *table,
                      uint64         key,
                      uint32         keylen,
                      uint64         value,
                      uint32         valuetype) {

  int32        hashkey   = (*table->hash)(key, keylen);
  int          bucket    = hashkey & table->hashmask;
  HashNode_AS *node      = NULL;

  assert(bucket >= 0);
  assert(bucket <= table->numBuckets);

  node = table->buckets[bucket];
  while (node) {
    int comparison = (*table->compare)(node->key, key);

    if (comparison == 0) {
      node->value     = value;
      node->valueType = valuetype;
      return(HASH_SUCCESS);
    }

    node = node->next;

    if (comparison < 0)
      //  The nodes are ordered by key, and we're past where it should be.
      node = NULL;
  }

  return(InsertInHashTable_AS(table, key, keylen, value, valuetype));
}






//  Lookup a key, and return its value.  Returns FALSE if the key isn't
//  found, TRUE if it is found.
//
int
LookupInHashTable_AS(HashTable_AS *table,
                     uint64        key,
                     uint32        keylen,
                     uint64       *value,
                     uint32       *valuetype) {

  int32        hashkey   = (*table->hash)(key, keylen);
  int          bucket    = hashkey & table->hashmask;
  HashNode_AS *node      = NULL;

#if 0
  if (keylen > 0)
    fprintf(stderr, "lookup - key "F_U64" %d,%d,%d keylen %d hashkey %d bucket %d\n",
            key,
            ((int *)key)[0],
            ((int *)key)[1],
            ((int *)key)[2],
            keylen,
            hashkey,
            bucket);
#endif

  assert(bucket >= 0);
  assert(bucket <= table->numBuckets);

  node = table->buckets[bucket];
  while (node) {
    int comparison = (*table->compare)(node->key, key);

    if (comparison == 0) {
      if (value)
        *value      = node->value;
      if (valuetype)
        *valuetype  = node->valueType;
      return(TRUE);
    }

    node = node->next;

    if (comparison < 0)
      //  The nodes are ordered by key, and we're past where it should be.
      node = NULL;
  }

  if (value)
    *value     = 0;
  if (valuetype)
    *valuetype = 0;
  return(FALSE);
}


int
ExistsInHashTable_AS(HashTable_AS *table,
                     uint64        key,
                     uint32        keylen) {
  return(LookupInHashTable_AS(table, key, keylen, NULL, NULL));
}

uint64
LookupValueInHashTable_AS(HashTable_AS *table,
                          uint64        key,
                          uint32        keylen) {
  uint64  val = 0;
  LookupInHashTable_AS(table, key, keylen, &val, NULL);
  return(val);
}

uint32
LookupTypeInHashTable_AS(HashTable_AS *table,
                         uint64        key,
                         uint32        keylen) {
  uint32  typ = 0;
  LookupInHashTable_AS(table, key, keylen, NULL, &typ);
  return(typ);
}




//  Offset all the key values (assumed to be pointers to some array).
//  This can be used when the array holding the data gets reallocated.
//
//  The alternative was to start using the Heap type, but that then
//  transfers complexity (LOTS) to the end algorithm that is expecting
//  an array or items.
//
void
UpdatePointersInHashTable_AS(HashTable_AS *table, int64 difference) {
  HeapIterator_AS  iter;
  HashNode_AS     *node;

  InitHeapIterator_AS(table->nodeheap, &iter);

  node = (HashNode_AS *)NextHeapIterator_AS(&iter);

  while (node) {
    if (node->isFree == 0)
      node->key += difference;
    node = (HashNode_AS *)NextHeapIterator_AS(&iter);
  }
}





//  At present, we support saving ONLY scalar hash tables -- we cannot
//  save hash tables where the key is a pointer to some user data.

void
SaveHashTable_AS(char *name, HashTable_AS *table) {
  HeapIterator_AS             iterator;
  HashNode_AS                *node;

  uint32                  databufferlen = 0;
  uint32                  databuffermax = 1048576 / sizeof(HashNode_AS);
  HashNode_AS            *databuffer    = (HashNode_AS *)safe_malloc(databuffermax * sizeof(HashNode_AS));

  uint32                  actualNodes = 0;

  FILE                   *fp;

  if (table->filename != name)
    strcpy(table->filename, name);

  errno = 0;
  fp = fopen(table->filename, "w");
  if (errno)
    fprintf(stderr, "failed to open HashTable_AS '%s': %s\n", table->filename, strerror(errno)), exit(1);

  AS_UTL_safeWrite(fp, &table->numBuckets, "SaveHashTable_AS numBuckets", sizeof(uint32), 1);
  AS_UTL_safeWrite(fp, &table->numNodes,   "SaveHashTable_AS numNodes",   sizeof(uint32), 1);
  AS_UTL_safeWrite(fp, &table->maxNodes,   "SaveHashTable_AS maxNodes",   sizeof(uint32), 1);
  AS_UTL_safeWrite(fp, &table->hashmask,   "SaveHashTable_AS hashmask",   sizeof(uint32), 1);
  AS_UTL_safeWrite(fp, &actualNodes,       "SaveHashTable_AS header",     sizeof(uint32), 1);

  InitHeapIterator_AS(table->nodeheap, &iterator);
  while (NULL != (node = (HashNode_AS *)NextHeapIterator_AS(&iterator))) {
    if (node->isFree == 0) {
      if (databufferlen >= databuffermax) {
        AS_UTL_safeWrite(fp, databuffer, "SaveHashTable_AS writedata", sizeof(HashNode_AS), databufferlen);
        databufferlen = 0;
      }

      databuffer[databufferlen] = *node;
      databufferlen++;
      actualNodes++;
    }
  }

  if (databufferlen > 0) {
    AS_UTL_safeWrite(fp, databuffer, "SaveHashTable_AS writedata", sizeof(HashNode_AS), databufferlen);
    databufferlen = 0;
  }

  rewind(fp);
  AS_UTL_safeWrite(fp, &table->numBuckets, "SaveHashTable_AS numBuckets",  sizeof(uint32), 1);
  AS_UTL_safeWrite(fp, &table->numNodes,   "SaveHashTable_AS numNodes",    sizeof(uint32), 1);
  AS_UTL_safeWrite(fp, &table->maxNodes,   "SaveHashTable_AS maxNodes",    sizeof(uint32), 1);
  AS_UTL_safeWrite(fp, &table->hashmask,   "SaveHashTable_AS hashmask",    sizeof(uint32), 1);
  AS_UTL_safeWrite(fp, &actualNodes,       "SaveHashTable_AS actualNodes", sizeof(uint32), 1);

  if (fclose(fp))
    fprintf(stderr, "SaveHashTable_AS()-- failed to close the hash table file '%s': %s\n", name, strerror(errno)), exit(1);

  safe_free(databuffer);

  table->dirty = 0;
}


HashTable_AS *
LoadHashTable_AS(char *name,
                 ASHashHashFn  hashfn,
                 ASHashCompFn  compfn) {

  uint32                databufferlen = 0;
  uint32                databuffermax = 1048576 / sizeof(HashNode_AS);
  HashNode_AS          *databuffer    = (HashNode_AS *)safe_malloc(databuffermax * sizeof(HashNode_AS));

  uint32                actualNodes = 0;

  HashTable_AS         *table= (HashTable_AS *)safe_calloc(1, sizeof(HashTable_AS));

  FILE                 *fp;

  errno = 0;
  fp = fopen(name, "r");
  if (errno)
    fprintf(stderr, "failed to open HashTable_AS '%s': %s\n", name, strerror(errno)), exit(1);

  AS_UTL_safeRead(fp, &table->numBuckets, "LoadHashTable_AS numBuckets",  sizeof(uint32), 1);
  AS_UTL_safeRead(fp, &table->numNodes,   "LoadHashTable_AS numNodes",    sizeof(uint32), 1);
  AS_UTL_safeRead(fp, &table->maxNodes,   "LoadHashTable_AS maxNodes",    sizeof(uint32), 1);
  AS_UTL_safeRead(fp, &table->hashmask,   "LoadHashTable_AS hashmask",    sizeof(uint32), 1);
  AS_UTL_safeRead(fp, &actualNodes,       "LoadHashTable_AS actualNodes", sizeof(uint32), 1);

  table->buckets           = (HashNode_AS **)safe_calloc(table->numBuckets, sizeof(HashNode_AS *));
  table->freeList          = NULL;
  table->nodeheap          = AllocateHeap_AS(sizeof(HashNode_AS));
  table->dirty             = 0;
  table->compare           = compfn;
  table->hash              = hashfn;

  //  The table has no nodes in it; we add them now.
  table->numNodes = 0;

  strcpy(table->filename, name);

  while (actualNodes > 0) {
    uint32  i;
    uint32  l = MIN(databuffermax, actualNodes);

    databufferlen = AS_UTL_safeRead(fp, databuffer, "LoadHashTable_AS writedata", sizeof(HashNode_AS), l);

    for (i=0; i<databufferlen; i++)
      InsertInHashTable_AS(table, databuffer[i].key, databuffer[i].keyLength, databuffer[i].value, databuffer[i].valueType);

    actualNodes -= databufferlen;
  }

  fclose(fp);
  safe_free(databuffer);

  //  The InsertInHashTable_AS call above marks our table as dirty,
  //  even though it isn't.
  table->dirty = 0;

  return(table);
}

HashTable_AS *
LoadUIDtoIIDHashTable_AS(char *name) {
  return(LoadHashTable_AS(name, INThashfunction, INTcomparefunction));
}



// HashTable_Iterator_AS
//    Iterator for all allocated nodes in hashtable.
//    In the absence of frees, values are returned in the order inserted
//
void
InitializeHashTable_Iterator_AS(HashTable_AS *table,
                                HashTable_Iterator_AS *iterator) {
  iterator->table = table;
  InitHeapIterator_AS(table->nodeheap, &(iterator->iterator));
}

int
NextHashTable_Iterator_AS(HashTable_Iterator_AS *iterator,
                          uint64                *key,
                          uint64                *value,
                          uint32                *valuetype) {

  HashNode_AS *node = (HashNode_AS *)NextHeapIterator_AS(&iterator->iterator);

  while (node && node->isFree)
    node = (HashNode_AS *)NextHeapIterator_AS(&iterator->iterator);

  if (node == NULL) {
    *key        = 0;
    *value      = 0;
    *valuetype  = 0;

    return(HASH_FAILURE);
  }

  *key       = node->key;
  *value     = node->value;
  *valuetype = node->valueType;

  return(HASH_SUCCESS);
}

