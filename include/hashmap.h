#ifndef HASHMAP_H
#define HASHMAP_H

#include <pthread.h>
#include "murmur3.h"
#include <string.h>
#include "limits.h"
#include <stddef.h>

// JESSES HASHMAP

enum HASH_MAP_STORE_FLAGS {
    HM_STORE_FREEDATA = 1, HM_STORE_KEYDUP = 2
};

typedef struct {
    void* data;
    size_t* index; //-- empty ptr means tombstone
    const char* key;
    enum HASH_MAP_STORE_FLAGS flags;
} entry_t;

unsigned char is_tomb(entry_t* e);

enum hm_mutex_index{
    HM_MUT_DATA, HM_MUT_REHASH1, HM_MUT_REHASH2, HM_MUT_BLOCKER, N_HM_MUTEXES
};

typedef struct {
    size_t n_entries, cap;
    size_t (*hash_func)(const char* key);
    entry_t* data;   //declare this extern
    pthread_mutex_t mutex[N_HM_MUTEXES];
    unsigned blockers;
} hashmap_t;

void* hm_data(hashmap_t* hm, size_t index) ;

hashmap_t* new_hm();

void hm_delete(hashmap_t* hm, size_t index) ;

void delete_hm(hashmap_t* hm);

//DO NOT ASSIGN NEW ENTRIES DURING BLOCK!!!!
void hm_block_rehash(hashmap_t* hm);

//DO NOT ASSIGN NEW ENTRIES DURING BLOCK!!!!
void hm_unblock_rehash(hashmap_t* hm);

typedef struct {
    size_t index;
    entry_t* p;
} queres_t;

void hm_hit2(hashmap_t* hm,const char* key,queres_t* res) ;

void hm_hit(hashmap_t* hm,const char* key,queres_t* res);

const char* hm_get(hashmap_t* hm,const char* key) ;

#ifdef DEBUG
#include "log.h"
    void hm_log_all(hashmap_t* hm);
#endif

void hm_rehash(hashmap_t* hm);

size_t* hm_set(hashmap_t* hm,const char* key, void* data, enum HASH_MAP_STORE_FLAGS flags);

size_t* hm_get_it(hashmap_t* hm,const char* key);

#endif //HASHMAP_H