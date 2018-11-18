#include "hashtable.h"
#include "MemRoot.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern ub4 iFreeKeys;

/* Function: Constructor
 * Description: Initializes the hash table to be of size
 * 128 (2^7).
 */
HashTable::HashTable()
{
    iLogSize = 7;
    Init();
}

/* Function: Constructor
 * Inputs: an int representing the log2 of the size of the table, 
 * so a log size of 1 makes a table of size 2; 2->4, 3->8, etc.
 * Description: Initializes the hast table to be of size
 * 2^InitialLogSize.
 */
HashTable::HashTable(int iInitialLogSize)
{
    iLogSize = iInitialLogSize;
    Init();
}

/* Function: Destructor
 * Description: Destroies the table;
 */
HashTable::~HashTable()
{

    for (unsigned int i = 0; i < ulTableSize; i++) {
        while (pTable[i]) {

            /* Modified by Chris Johnson, 6/6/2005.  The following two lines
             * are commented out because the hashtable has no clue how to free
             * complicated data structures.  The key would be fine to free,
             * but it doesn't free deleted items' keys, so commenting this line
             * makes its behavior more consistent.  It's up to the user to free
             * key and data buffers. */
            /* free(pTable[i]->pKey); */
            /* free(pTable[i]->pData); */
            /* End modifications. */

            pTable[i] = pTable[i]->pNext;
        }

    }

    delete pMemRoot;

    free(pTable);
}

/* Function: Init
 * Description: Allocates memory for the table, based on the log
 * size at the time the function is called.
 * This function is only called from the constructor, and is protected, so won't
 * be called from any other function.
 */
void HashTable::Init()
{

    ulTableSize = ((ub4) 1 << iLogSize);
    iMask = ulTableSize - 1;
    ulCount = 0;
    pTable = (HashEntry **) malloc(sizeof(HashEntry *) * ulTableSize);
    for (unsigned int i = 0; i < ulTableSize; i++)
        pTable[i] = (HashEntry *) 0;

    pMemRoot = new MemRoot(sizeof(HashEntry));
    pthread_mutex_init(&tableLock, NULL);
    return;
}

/* Function: grow
 * Description: Doubles the size of the table.  Copies all pointers from the old
 * table into the new table.  
 * Notes: Don't want this function to be called many times during a program's
 * run, as it is likely the slowest operation
 */
#include <stdio.h>
void HashTable::grow()
{
    register ub4 i = 0;
    register ub4 ulNewSize = ((ub4) 1 << ++iLogSize);
    register ub4 ulNewMask = ulNewSize - 1;
    register HashEntry **pNewTable =
        (HashEntry **) malloc(sizeof(HashEntry *) * ulNewSize);
    register HashEntry **pOldTable = pTable;

    for (unsigned int i = 0; i < ulNewSize; i++)
        pNewTable[i] = (HashEntry *) 0;

    for (i = ulNewSize >> 1; i--;) {
        register HashEntry *next, *curr, **loc;

        next = pOldTable[i];

        while (next != NULL) {
            curr = next;
            next = next->pNext;
            loc = &pNewTable[(curr->ulHashVal & ulNewMask)];
            curr->pNext = *loc;
            *loc = curr;
        }
    }
    pthread_mutex_lock(&tableLock);
    free(pOldTable);
    pTable = pNewTable;
    iMask = ulNewMask;
    ulTableSize = ulNewSize;
    pthread_mutex_unlock(&tableLock);
    return;
}

/* Function: Insert
 * Description: Hashes the passed key and inserts it into the table.
 */
int HashTable::insert(void *pKey, ub4 ulKeyLength, void *pData)
{
    register ub4 ulHashVal = hash((ub1 *) pKey, ulKeyLength, 0);
    register ub4 ulHashIndex = ulHashVal & iMask;
    register HashEntry *pHashItem = pTable[ulHashIndex];
    register HashEntry **pHashPtr = 0;

    // make sure item isn't present.
    for (; pHashItem != NULL;) {
        if ((ulHashVal == pHashItem->ulHashVal) &&
            (ulKeyLength == pHashItem->ulKeyLength) &&
            !(memcmp(pKey, pHashItem->pKey, ulKeyLength))) {
            return 0;
        }
        pHashItem = pHashItem->pNext;
    }

    pHashItem = (struct hashentry *) pMemRoot->NewMem();
    /*
       grow table if needed 
     */
    if (++ulCount > ((ub4) 1 << iLogSize)) {
        grow();
        ulHashIndex = (ulHashVal & iMask);
    }
    /*
       add the new key to the table 
     */

    pHashItem->pKey = pKey;
    pHashItem->ulKeyLength = ulKeyLength;
    pHashItem->pData = pData;
    pHashPtr = &pTable[ulHashIndex];
    pHashItem->pNext = *pHashPtr;
    pthread_mutex_lock(&tableLock);
    *pHashPtr = pHashItem;
    pthread_mutex_unlock(&tableLock);
    pHashItem->ulHashVal = ulHashVal;

    /* Added by Chris Johnson, Fri, April 8, 2005. */
    pCurrentEntry = pHashItem;
    /* End modification. */

    return 1;
}

int HashTable::insert( void *pKey, ub4 ulKeyLength, void *pData, unsigned int
uiDS)
{
    register ub4 ulHashVal = hash((ub1 *) pKey, ulKeyLength, 0);
    register ub4 ulHashIndex = ulHashVal & iMask;
    register HashEntry *pHashItem = pTable[ulHashIndex];
    register HashEntry **pHashPtr = 0;

    // make sure item isn't present.
    for (; pHashItem != NULL;) {
        if ((ulHashVal == pHashItem->ulHashVal) &&
            (ulKeyLength == pHashItem->ulKeyLength) &&
            !(memcmp(pKey, pHashItem->pKey, ulKeyLength))) {
            return 0;
        }
        pHashItem = pHashItem->pNext;
    }

    pHashItem = (struct hashentry *) pMemRoot->NewMem();
    /*
       grow table if needed 
     */
    if (++ulCount > ((ub4) 1 << iLogSize)) {
        grow();
        ulHashIndex = (ulHashVal & iMask);
    }
    /*
       add the new key to the table 
     */

    pHashItem->pKey = pKey;
    pHashItem->ulKeyLength = ulKeyLength;
    pHashItem->pData = pData;
    pHashItem->iDataSize = uiDS;
    pHashPtr = &pTable[ulHashIndex];
    pHashItem->pNext = *pHashPtr;
    pthread_mutex_lock(&tableLock);
    *pHashPtr = pHashItem;
    pthread_mutex_unlock(&tableLock);
    pHashItem->ulHashVal = ulHashVal;
    return 1;


}
/* Function: find
 * Description: Locates the key in the table.
 * Returns: 1 on a find, 0 on a failure.
 */
int HashTable::find(void *key, ub4 ulKeyLength)
{

    ub4 ulHashVal = hash((ub1 *) key, ulKeyLength, 0);
    pthread_mutex_lock(&tableLock);
    ub4 ulHashIndex = ulHashVal & iMask;
    register HashEntry *pEnt = pTable[ulHashIndex];
    pthread_mutex_unlock(&tableLock);

    while (pEnt) {
        if ((ulHashVal == pEnt->ulHashVal) &&
            (ulKeyLength == pEnt->ulKeyLength) &&
            !(memcmp(key, pEnt->pKey, ulKeyLength))) {
            pCurrentEntry = pEnt;
            return 1;
        }
        pEnt = pEnt->pNext;
    }
    return 0;
}

#include <stdio.h>
#include <iostream>
using namespace std;
int HashTable::find(HashEntry **pRet, void * key, ub4 ulKeyLength)
{
	ub4 ulHashVal = hash((ub1 *) key, ulKeyLength, 0);
	pthread_mutex_lock(&tableLock);
	ub4 ulHashIndex = ulHashVal & iMask;
	register HashEntry * pEnt = pTable[ulHashIndex];
	pthread_mutex_unlock(&tableLock);

	while(pEnt)
	{
		if((ulHashVal == pEnt->ulHashVal) &&
				(ulKeyLength == pEnt->ulKeyLength) &&
				!(memcmp(key, pEnt->pKey, ulKeyLength)))
		{
			(*pRet) = pEnt;
			return 1;
		} 
		pEnt = pEnt->pNext;
	}
/*
	cout<<"Find of "<<(char *) key<<" failed"<<endl;
	pEnt = pTable[ulHashIndex];
	cout<<"Possibilities were: "<<endl;
	while(pEnt){
		cout<<(char *) pEnt->pKey<<endl;
		pEnt = pEnt->pNext;
	}
*/
	return 0;
}
#include <unistd.h>
void HashTable::statistics()
{
    ub4 i, j;
    double total = 0.0;
    HashEntry *h;
    HashEntry *walk, *walk2, *stats = 0;

    for (i = 0; i <= iMask; i++) {
        for (h = pTable[i], j = 0; h; j++, h = h->pNext);
        for (walk = stats; walk && (walk->ulKeyLength != j);
             walk = walk->pNext);
	/*
           if(j >= 6)
           {
           printf("The following keys clash\n");
           for(h = pTable[i];h ; h=h->pNext){
           write(1, ((char *)h->pKey), h->ulKeyLength);
           write(1, "\n", strlen("\n"));
           }
           }
	  */ 
        if (walk) {
            walk->ulHashVal++;
        } else {
            walk = (HashEntry *) pMemRoot->NewMem();
            walk->ulKeyLength = j;
            walk->ulHashVal = 1;
            if (!stats || stats->ulKeyLength > j) {
                walk->pNext = stats;
                stats = walk;
            } else {
                for (walk2 = stats;
                     walk2->pNext && (walk2->pNext->ulKeyLength < j);
                     walk2 = walk2->pNext);
                walk->pNext = walk2->pNext;
                walk2->pNext = walk;
            }
        }
    }
    for (walk = stats; walk; walk = walk->pNext) {
        total +=
            (double) walk->ulHashVal * (double) walk->ulKeyLength *
            (double) walk->ulKeyLength;
    }
    if (ulCount)
        total /= (double) ulCount;
    else
        total = 0.0;

    for (walk = stats; walk; walk = walk->pNext) {
        printf("items %ld: %ld buckets\n", walk->ulKeyLength, walk->ulHashVal);
    }
    printf("\nbuckets: %ld items: %ld existing: %g\n\n",
           ((ub4) 1 << iLogSize), ulCount, total);
    while (stats) {
        walk = stats->pNext;
        pMemRoot->DelMem((MemEnt *) stats);
        stats = walk;
    }
    return;
}

/*Function: hash
 * Description:
  --------------------------------------------------------------------
  hash() -- hash a variable-length key into a 32-bit value
    k       : the key (the unaligned variable-length array of bytes)
    len     : the length of the key, counting by bytes
    initval : can be any 4-byte value
    Returns a 32-bit value.  Every bit of the key affects every bit of
    the return value.  Every 1-bit and 2-bit delta achieves avalanche.
    About 6*len+35 instructions.
    The best hash table sizes are powers of 2.  There is no need to do
    mod a prime (mod is sooo slow!).  If you need less than 32 bits,
    use a bitmask.  For example, if you need only 10 bits, do
    h = (h & hashmask(10));
   In which case, the hash table should have hashsize(10) elements.
 
    If you are hashing n strings (ub1 **)k, do it like this:
    for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);
 
    By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  
    You may use this code any way you wish, private, educational, or commercial.
    It's free. See http://burtleburtle.net/bob/hash/evahash.html  
    Use for hash table lookup, or anything where one collision in 
    2^^32 is acceptable.  Do NOT use for cryptographic purposes.
 --------------------------------------------------------------------
  Returns: the table index of the key hashed.
 */
ub4 HashTable::hash(register ub1 * k, register ub4 length, register ub4 initval)
  /*
     register ub1 *k;            the key 
     register ub4 length;        the length of the key 
     register ub4 initval;       the previous hash, or an arbitrary value 
   */
{
    register ub4 a, b, c, len;

    /*
       Set up the internal state 
     */
    len = length;
    a = b = 0x9e3779b9;         /* the golden ratio; an arbitrary value */
    c = initval;                /* the previous hash value */

               /*---------------------------------------- handle most of
			* the key */
    while (len >= 12) {
        a += (k[0] + ((ub4) k[1] << 8) + ((ub4) k[2] << 16) +
              ((ub4) k[3] << 24));
        b += (k[4] + ((ub4) k[5] << 8) + ((ub4) k[6] << 16) +
              ((ub4) k[7] << 24));
        c += (k[8] + ((ub4) k[9] << 8) + ((ub4) k[10] << 16) +
              ((ub4) k[11] << 24));
        mix(a, b, c);
        k += 12;
        len -= 12;
    }

    /*------------------------------------- handle the last 11 bytes */
    c += length;
    switch (len) {              /* all the case statements fall through */
      case 11:
          c += ((ub4) k[10] << 24);
      case 10:
          c += ((ub4) k[9] << 16);
      case 9:
          c += ((ub4) k[8] << 8);
          /*
             the first byte of c is reserved for the length 
           */
      case 8:
          b += ((ub4) k[7] << 24);
      case 7:
          b += ((ub4) k[6] << 16);
      case 6:
          b += ((ub4) k[5] << 8);
      case 5:
          b += k[4];
      case 4:
          a += ((ub4) k[3] << 24);
      case 3:
          a += ((ub4) k[2] << 16);
      case 2:
          a += ((ub4) k[1] << 8);
      case 1:
          a += k[0];
          /*
             case 0: nothing left to add 
           */
    }
    mix(a, b, c);

   /*--------------------------------------  report the result */
    return c;
}

bool HashTable::deleteItem(void * key ,ub4 keylen )
{
	HashEntry * pItem;
	
	HashEntry ** ppItem;


	if(!find(&pItem, key, keylen)) return false;;

	

	for(ppItem = &pTable[pItem->ulHashVal & iMask]; *ppItem != pItem;
		ppItem = &(*ppItem)->pNext);
	*ppItem = (*ppItem)->pNext;

	ulCount--;

	pMemRoot->DelMem((MemEnt *)pItem);

	return true;
}

