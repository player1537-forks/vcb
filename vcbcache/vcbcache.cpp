/* ------------------------------------------------------------------------- */
/* FILE:   vcbcache.cpp                                                      */
/* AUTHOR: Chris Johnson                                                     */
/* DATE:   Apr 08 2005                                                       */
/*                                                                           */
/* This file implements a caching system for an arbitrary collection of      */
/* elements.  Data-specific functions like reading data on a cache miss      */
/* and key display must be provided by the user.                             */
/* ------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "hashtable.h"
#include "vcbcache.h"

extern "C" {
#include "jrb.h"
#include "dllist.h"
}

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */

/* Data structure for each element in cache. */
typedef struct {
   JRB time_node;             /* Element's node in the cache's time tree. */
   void *data;                /* The actual data tied to the key. */
   vcbCacheKeyData *key_data; /* Key used to insert element in hash table. */
   int size;                  /* Number of bytes consumed by cache element. */
} cache_element_t;

/* Data structure for prefetch items: key plus time originally requested. */
typedef struct {
   vcbCacheKeyData *key_data;  /* Item's key data. */
   int priority;               /* Rank/priority when vcbCacheFetch was called.
                                * May be fetched at a later time, so this is
                                * stored. */
} prefetch_item_t;

/* Data structure for entire cache. */
typedef struct {
   HashTable *htable;           /* The hashtable where elements are stored. */
   int capacity;                /* The maximum capacity of the cache. */
   int fullness;                /* The current number of elements in cache. */
   int num_purge;               /* # of elements to drop on capacity miss. */
   int num_hits;                /* Number of reads that were hits. */
   int num_misses;              /* Number of reads that were misses. */
   int max_prefetch;            /* Maximum number of data to prefetch. */
   int num_prefetch;            /* Current number of items to prefetch. */
   int size;                    /* Number of bytes consumed by cache. */
   JRB time_tree;               /* A tree of cached elements used for finding
                                   least recently used element: O(log n). */

   Dllist prefetch_list;        /* List of items to prefetch. */
   pthread_mutex_t lock;        /* Provide mutually exclusive cache access. */
   pthread_cond_t can_prefetch; /* Signalled means non-empty prefetch list. */
   int no_prefetch;             /* Indicates prefetch should take a backseat.*/
   pthread_t tid;               /* Prefetching thread id. */

   /* A user-supplied function to read missing data into the cache. */
   void *(* non_cache_read) (vcbCacheKeyData *key_data, int *size); 

   /* A user-supplied function to release the cached data. */
   void (* free_data) (void *data);

   /* A user-supplied function to release the cached data. */
   void (* free_key) (vcbCacheKeyData *key);

   /* A user-supplied function that interprets and displays the key. */
   void (* print_key) (vcbCacheKeyData *key_data);
} cache_t;

/* ------------------------------------------------------------------------- */
/* FUNCTION PROTOTYPES                                                       */
/* ------------------------------------------------------------------------- */

void purge_prefetch_list(cache_t *cache, int num_to_remove);
void *prefetch(void *arg);
void *read_element(cache_t *cache, vcbCacheKeyData *key_data, time_t now,
                   int prefetch);

/* ------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS                                                          */
/* ------------------------------------------------------------------------- */

vcbCache vcbCacheMake(void *(* non_cache_read) (vcbCacheKeyData *key_data, int *size),
                      void (* free_data) (void *data),
                      void (* free_key) (vcbCacheKeyData *key_data),
                      void (* print_key) (vcbCacheKeyData *key_data),
                      int maximum_capacity,
                      int num_purge,
                      int max_num_to_prefetch) {

   int failed;        /* Indicates failure of function calls. */
   cache_t *cache;    /* The cache that is made and of which a handle is
                       * returned to the user. */

   /* Verify integrous data. */
   if (maximum_capacity < 1) {
      fprintf(stderr, "vcbCacheMake: cache capacity must be at least 1 "
                      "byte.\n");
      exit(1);
   }

   if (num_purge < 1) {
      fprintf(stderr, "vcbCacheMake: purges must kick out at least 1 "
                      " byte.\n");
      exit(1);
   }

   /* We don't want to kick out more elements than we could possible have. */
   if (num_purge > maximum_capacity) {
      fprintf(stderr, "vcbCacheMake: cache purges cannot kick out more\n"
                      "bytes than the cache can hold! (%d > %d)\n", 
                      num_purge, maximum_capacity);
      exit(1);
   }

   /* Make sure max_prefetch is >= 0. */
   if (max_num_to_prefetch < 0) {
      fprintf(stderr, "vcbCacheMake: maximum number of items to prefetch\n"
                      "must be 0 or more.\n");
      exit(1);
   }

   /* Make and initialize the cache.  The user will get back a cache handle
    * that will be used for further cache manipulation. */
   cache = (cache_t *) malloc(sizeof(cache_t));
   if (cache == NULL) {
      perror("vcbCacheMake: couldn't allocate space for cache.\n");
      exit(1);
   }

   cache->max_prefetch = max_num_to_prefetch;
   cache->num_prefetch = 0;
   cache->num_purge = num_purge;
   cache->htable = new HashTable();
   cache->capacity = maximum_capacity;
   cache->fullness = 0;
   cache->time_tree = make_jrb();
   cache->num_hits = 0;
   cache->num_misses = 0;
   cache->non_cache_read = non_cache_read;
   cache->print_key = print_key;
   cache->free_data = free_data;
   cache->free_key = free_key;
   cache->prefetch_list = new_dllist();
   cache->no_prefetch = 0;

   /* Make mutex and condition variable. */
   failed = pthread_mutex_init(&(cache->lock), NULL);
   if (failed) {
      perror("vcbCacheMake: couldn't create mutex");
      exit(1);
   }

   failed = pthread_cond_init(&(cache->can_prefetch), NULL);
   if (failed) {
      perror("vcbCacheMake: couldn't create condition variable");
      exit(1);
   }

   /* Shoot off a thread that handles all prefetching in the background. */
   failed = pthread_create(&(cache->tid), NULL, prefetch, (void *) cache);
   if (failed) {
      perror("vcbCacheMake: couldn't spawn prefetching thread");
      exit(1);
   }

   return (vcbCache) cache;
}

/* ------------------------------------------------------------------------- */

void vcbCacheReset(vcbCache cache) {

   JRB tmp;                      /* Traversing tree node. */
   cache_element_t *element;     /* Cache element at each node in tree. */
   cache_t *c;                   /* The cache itself. */

   /* User just gave us a void * (vcbCache).  Cast it to something useful. */
   c = (cache_t *) cache;

   /* Free up the items in the prefetch list.  The prefetch thread's still
    * running, so this is the first thing we do (to limit that thread's 
    * now unnecessary computation) and we must lock the cache down. */
   c->no_prefetch = 1;
   if (pthread_mutex_lock(&(c->lock))) {
      perror("vcbCacheReset: couldn't lock cache");
      exit(1);
   }

   purge_prefetch_list(c, c->num_prefetch);
   c->no_prefetch = 0;

   if (pthread_mutex_unlock(&(c->lock))) {
      perror("vcbCacheReset: couldn't unlock cache");
      exit(1);
   }

   /* Free all cache elements' data and keys, and remove them from the
    * hashtable. */
   jrb_traverse(tmp, c->time_tree) {
      element = (cache_element_t *) tmp->val.v;
      c->htable->deleteItem(element->key_data->key, element->key_data->len);
      c->free_data(element->data);
      c->free_key(element->key_data);
   }

   /* We're done traversing the cache, which we needed the tree for, so we
    * can release all tree elements.  This is easiest done by releasing and
    * allocating a whole new tree. */
   jrb_free_tree(c->time_tree);
   c->time_tree = make_jrb();

   /* Set cache stats back to 0. */
   c->num_hits = 0;
   c->num_misses = 0;
   c->fullness = 0;
   c->num_prefetch = 0;

}

/* ------------------------------------------------------------------------- */

void vcbCacheDestroy(vcbCache cache) {

   JRB tmp;                      /* Traversing tree node. */
   cache_element_t *element;     /* Cache element at each node in tree. */
   cache_t *c;                   /* The cache itself. */
   int failed;                   /* Indicates failure of function call. */

   /* User just gave us a void * (vcbCache).  Cast it to something useful. */
   c = (cache_t *) cache;

   /* Kill the prefetching thread, but don't interrupt the item currently
    * being prefetched (it may be doing file reads and so on, which are best
    * not too interrupt). */
   c->no_prefetch = 1;
   if (pthread_mutex_lock(&(c->lock))) {
      perror("vcbCacheDestroy: couldn't lock cache");
      exit(1);
   }
   failed = pthread_kill(c->tid, SIGQUIT);
   if (failed) {
      perror("vcbCacheDestroy: couldn't kill prefetching thread");
      exit(1);
   }
   if (pthread_mutex_unlock(&(c->lock))) {
      perror("vcbCacheDestroy: couldn't unlock cache");
      exit(1);
   }

   /* Free mutex and condition variable. */
   if ((failed = pthread_mutex_destroy(&(c->lock)))) {
      perror("vcbCacheDestroy: couldn't release lock");
      exit(1);
   }

   if (pthread_cond_destroy(&(c->can_prefetch))) {
      perror("vcbCacheDestroy: couldn't release condition variable");
      exit(1);
   }

   /* Free up the items in the prefetch list. */
   purge_prefetch_list(c, c->num_prefetch);
   free_dllist(c->prefetch_list);

   /* Free all cache elements' data and keys, and remove them from the
    * hashtable. */
   jrb_traverse(tmp, c->time_tree) {
      element = (cache_element_t *) tmp->val.v;
      c->htable->deleteItem(element->key_data->key, element->key_data->len);
      c->free_data(element->data);
      c->free_key(element->key_data);
   }

   /* We're totally done with the hashtable and tree, so we free up their
    * resources. */
   delete c->htable;
   jrb_free_tree(c->time_tree);

}

/* ------------------------------------------------------------------------- */

void *vcbCacheFetch(vcbCache cache, vcbCacheKeyData **keys, int num_elements) {

   int i;                 /* Simple counter. */
   void *request = NULL;  /* Data tied to first key in list. */
   time_t now;            /* Current clock time. */
   cache_t *c;            /* The cache itself. */
   prefetch_item_t *item; /* Item to prefetch. */

   c = (cache_t *) cache;

   /* Grab the time once, not for each key. */
   now = time(NULL);

   /* First we grab the data for the requested element.  It will have the
    * access time farthest into the future, giving it priority when elements
    * are chosen to be kicked out.  This is helpful for two reasons: a)
    * prefetched elements may not be used, while specifically requested
    * elements are likely to be revisited, and b) prefetched elements won't
    * kick out the requested element like they might if all elements in the
    * list were given the same priority. */
   if (num_elements > 0) {
      c->no_prefetch = 1;

      if (pthread_mutex_lock(&(c->lock))) {
         perror("vcbCacheFetch: couldn't lock cache");
         exit(1);
      }

      request = read_element(c, keys[0], now + num_elements, 0);

      /* Then we handle prefetching.  The farther we get from the requested
       * element, the less priority (time) we give the element, so it's more
       * likely to be kicked out than closer and possibly more important
       * elements.  */
      if (num_elements > 1) {

         /* Make room for new items in prefetch list, if we'll exceed max. */
         if (c->max_prefetch > 0) {
            i = c->num_prefetch + num_elements - 1;
            if (i > c->max_prefetch) {
               purge_prefetch_list(c, i - c->max_prefetch);
            }
         }

         /* Create copies of all the keys to prefetch so this memory can be
          * freed by user.  We need our own copies because who knows when we'll
          * be done with the memory 'cuz of pthread's preemptive nature.
          * We prepend the prefetch items in reverse order, causing the item
          * listed as most important from this fetch to be considered for
          * prefetching next.  By prepending all prefetch items from this
          * fetch, we have these considered before prefetches from other
          * fetches.  The priorities assigned to them ultimately determine
          * which will actually be read, but the order in the list determines
          * when they'll be considered. */
         for (i = num_elements - 1; i >= 1; i--) {
            /* Make memories. */
            item = (prefetch_item_t *) malloc(sizeof(prefetch_item_t));
            if (item == NULL) {
               perror("vcbCacheMake: couldn't allocate space.\n");
               exit(1);
            }

            item->key_data =
               (vcbCacheKeyData *) malloc(sizeof(vcbCacheKeyData));
            if (item->key_data == NULL) {
               perror("vcbCacheMake: couldn't allocate space.\n");
               exit(1);
            }

            item->key_data->key = malloc(keys[i]->len);
            if (item->key_data->key == NULL) {
            perror("vcbCacheMake: couldn't allocate space.\n");
               exit(1);
            }

            /* Assign the values to the new memory. */
            item->key_data->len = keys[i]->len;
            memcpy(item->key_data->key, keys[i]->key, keys[i]->len);
            item->priority = now + num_elements - i;

            /* Add the data to the start of the list. */
            dll_prepend(c->prefetch_list, new_jval_v((void *) item));
            c->num_prefetch++;
         }

         /* Let prefetch thread know there's something to prefetch. */
         if (pthread_cond_signal(&(c->can_prefetch))) {
            perror("vcbCacheFetch: couldn't signal condition");
            exit(1);
         }

         c->no_prefetch = 0;

      }

      if (pthread_mutex_unlock(&(c->lock))) {
         perror("vcbCacheFetch: couldn't unlock cache");
         exit(1);
      }
   }

   return request;

}

/* ------------------------------------------------------------------------- */

void vcbCacheStats(vcbCache cache) {

   JRB tmp;                   /* Traversing node. */
   cache_element_t *element;  /* Cache element at each node. */
   cache_t *c;                /* The cache itself. */

   /* Cast cache handle to something useful. */
   c = (cache_t *) cache;

   if (pthread_mutex_lock(&(c->lock))) {
      perror("vcbCacheFetch: couldn't lock cache");
      exit(1);
   }

   /* Run through cache by time of last access, printing out the key with the
    * user's print_key function and the time of last access. */
   printf("\n----------------------\nCache Statistics\n\n");
   jrb_traverse(tmp, c->time_tree) {
      element = (cache_element_t *) tmp->val.v;
      printf("element (");
      c->print_key(element->key_data);
      printf(") last accessed at %d, size %d\n", tmp->key.i, element->size);
   }

   /* Show some usage statistics. */
   printf("Cache fullness: %d/%d (%f%%)\n", c->fullness, c->capacity,
                                            100.0f * c->fullness / c->capacity);
   printf("Hit rate: %f\n", (double) c->num_hits /
                            (c->num_hits + c->num_misses));
   printf("Miss rate: %f\n", (double) c->num_misses /
                             (c->num_hits + c->num_misses));
   printf("----------------------\n\n");

   if (pthread_mutex_unlock(&(c->lock))) {
      perror("vcbCacheFetch: couldn't unlock cache");
      exit(1);
   }

}

/* ------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS                                                           */
/* ------------------------------------------------------------------------- */

void *read_element(cache_t *cache, vcbCacheKeyData *key_data, time_t now,
                   int prefetch) {

   int i;                     /* Counter. */
   cache_element_t *element;  /* Corresponding cache element. */
   cache_element_t *oldest;   /* Oldest cache element. */
   JRB tmp;                   /* Oldest node in time tree. */
   int num_bytes_to_purge;    /* Number of bytes to remove from full cache. */

   /* If we're lucky, the element is already in the cache.  In that case, we
    * just update its access time and drop it in the time tree so that it
    * will be resorted. */
   if (cache->htable->find(key_data->key, key_data->len)) {

      cache->num_hits++;

      /* The hashtable's find function points a class variable to the found
       * node.  We access it with getCurrent(). */
      element = (cache_element_t *) cache->htable->getCurrent()->pData;

      /* We remove the node only from the time tree, because its keyed on
       * a stale key.  We reinsert it in the current time, the new latest
       * access time. */
      jrb_delete_node(element->time_node);
      element->time_node = jrb_insert_int(cache->time_tree, now,
                                       new_jval_v((void *) element));

      /* We free the key the user made for us since we already have it from
       * a previous fetch. */
      cache->free_key(key_data);
   }
   
   /* Or the block was not in the cache, in which case we have to add it and
    * kick out elements if the cache is too full. */
   else {

      cache->num_misses++;

      /* Create a new struct for a cache element.  Really we should do this
       * after we've made sure there's room for the element.  But we don't
       * know the size of the element until the non_cache_read call below.
       * So, we add it first, and pare down the cache after.  This shouldn't
       * be a big deal, unless the user is maxing out memory. */
      element = (cache_element_t *) malloc(sizeof(cache_element_t));
      if (element == NULL)
         return 0;

      /* Read in the element. */
      element->data = cache->non_cache_read(key_data, &(element->size));

      /* If we are at maximum capacity, we have kick some elements out.  We use
       * the red-black tree to find data with the access time farthest into the
       * past and then free its resources.  We remove blocks from the time tree
       * AND the hash table. */
      if (cache->fullness + element->size > cache->capacity) {

         /* We must kick out at least enough room for the new element.  If the
          * user requested more than the element, do that.  If the user 
          * requested less, ignore and kick out enough for the element. */
         if (cache->num_purge > element->size) {
            num_bytes_to_purge = cache->num_purge;
         } else {
            num_bytes_to_purge = element->size;
         }

         /* Kick out elements until we've purged the requested amount or there
          * aren't any elements left. */
         i = 0;
         while (i < num_bytes_to_purge && cache->fullness > 0) {
            tmp = jrb_first(cache->time_tree);
            oldest = (cache_element_t *) tmp->val.v;

            /* If the element is to be prefetched and the oldest element has
             * a higher priority, then we skip this element. */
            if (prefetch && tmp->key.i > now) {
               /* Need to release some memory here. */
               free(element->data);
               free(element);
               return NULL;
            }

            /* Otherwise, we can delete this node from the time tree and
             * the hashtable, and release its resources.  It is officially
             * kicked out. */
            jrb_delete_node(jrb_first(cache->time_tree));
            cache->htable->deleteItem(oldest->key_data->key,
                                      oldest->key_data->len);

            cache->free_data(oldest->data);
            cache->free_key(oldest->key_data);
            free(oldest);

            cache->fullness -= oldest->size;
            i += oldest->size;
         }

      }

      /* Insert it into the tree keyed on this time, and insert it into the
       * hashtable keyed on the user-supplied key. */
      element->time_node = jrb_insert_int(cache->time_tree, now,
                                          new_jval_v((void *) element));
      cache->htable->insert(key_data->key, key_data->len, (void *) element);

      /* When our cache gets too full and we pull the oldest block out of the
       * time tree, we need to remove it from the hash table using the key.
       * So, we store a pointer to the key for later use. */
      element->key_data = key_data;

      cache->fullness += element->size;

   }

   return element->data;

}

/* ------------------------------------------------------------------------- */

void purge_prefetch_list(cache_t *cache, int num_to_remove) {

   prefetch_item_t *item;    /* An item in the prefetch list. */

   /* Try to remove requested number of items from the prefetch list.  Stop
    * if the list becomes empty, however. */
   while (num_to_remove > 0 && !dll_empty(cache->prefetch_list)) {
      item = (prefetch_item_t *) dll_last(cache->prefetch_list)->val.v;
      free(item->key_data->key);
      free(item->key_data);
      free(item);
      dll_delete_node(dll_last(cache->prefetch_list));
      num_to_remove--;
      cache->num_prefetch--;
   }

}

/* ------------------------------------------------------------------------- */

void *prefetch(void *arg) {

   cache_t *c;               /* The cache to prefetch into. */
   prefetch_item_t *item;    /* An item to prefetch found in prefetch list. */

   c = (cache_t *) arg;

   /* Run forever, looking for things to prefetch. */
   while (1) {

      /* Lock cache access unless there's nothing to do (in which case we wait
       * for the vcbCacheFetch to signal us that there is something to do). */
      if (pthread_mutex_lock(&(c->lock))) {
         perror("vcbCache prefetching: couldn't lock cache");
         exit(1);
      }

      while (c->no_prefetch || dll_empty(c->prefetch_list)) {
         if (pthread_cond_wait(&(c->can_prefetch), &(c->lock))) {
            perror("vcbCache prefetching: couldn't wait on condition");
            exit(1);
         }
      }

      /* Grab the first item off the prefetch list and read it in. */
      item = (prefetch_item_t *) dll_first(c->prefetch_list)->val.v;
      read_element(c, item->key_data, item->priority, 1);

      /* Remove item from list and give cache access back to whoever needs
       * it. */
      free(item);
      dll_delete_node(dll_first(c->prefetch_list));
      c->num_prefetch--;

      if (pthread_mutex_unlock(&(c->lock))) {
         perror("vcbCache prefetching: couldn't unlock cache");
         exit(1);
      }

   }

   return (void *) NULL;

}

/* ------------------------------------------------------------------------- */

