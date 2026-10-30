#include "hashmap.h"

#include <pthread.h>
#include "murmur3.h"
#include "mem_dbg.h"
#include <string.h>
#include "limits.h"


// JESSES HASHMAP

unsigned char is_tomb(entry_t* e) {
    return !e->index;
}

void* hm_data(hashmap_t* hm, size_t index) {
    return hm->data[index].data;
}

static size_t default_hash(const char* key) {
    uint32_t buf
    #if SIZE_MAX > 0xFFFFFFFFu
        [4];
        MurmurHash3_x64_128
    #else
        [1];
        MurmurHash3_x86_32
    #endif
        (key,strlen(key),0,buf);
    return *(size_t*)buf;
}

hashmap_t* new_hm() {
    hashmap_t* hm = sw_malloc(sizeof(hashmap_t));
    hm->hash_func = default_hash;
    hm->data = sw_calloc(16,sizeof(entry_t));
    hm->n_entries = 0;
    hm->cap = 16;
    hm->blockers = 0;
    enum hm_mutex_index i = N_HM_MUTEXES;
    while (i--) {
        pthread_mutex_init(&hm->mutex[i],0);
    }
    return hm;
}

void hm_delete(hashmap_t* hm, size_t index) {
    entry_t* const o = &hm->data[index];
        if (is_tomb(o)); else {
            if (o->flags & HM_STORE_FREEDATA) sw_free(o->data);
            if (HM_STORE_KEYDUP & o->flags) sw_free(o->key);
            sw_free(o->index);
            o->index=0;
        }
}

void delete_hm(hashmap_t* hm) {
    for (size_t i = 0; i<hm->cap; i++) {
        hm_delete(hm,i);
    }
    sw_free(hm->data);
    sw_free(hm);
}

//DO NOT ASSIGN NEW ENTRIES DURING BLOCK!!!!
void hm_block_rehash(hashmap_t* hm) {
    pthread_mutex_lock(&hm->mutex[HM_MUT_BLOCKER]);
    if (!hm->blockers++) pthread_mutex_lock(&hm->mutex[HM_MUT_REHASH2]);
    pthread_mutex_unlock(&hm->mutex[HM_MUT_BLOCKER]);
}

//DO NOT ASSIGN NEW ENTRIES DURING BLOCK!!!!
void hm_unblock_rehash(hashmap_t* hm) {
    pthread_mutex_lock(&hm->mutex[HM_MUT_BLOCKER]);
    hm->blockers = (!!hm->blockers) * (hm->blockers-1);
    if (!hm->blockers) pthread_mutex_unlock(&hm->mutex[HM_MUT_REHASH2]);
    pthread_mutex_unlock(&hm->mutex[HM_MUT_BLOCKER]);
}

void hm_hit2(hashmap_t* hm,const char* key,queres_t* res) {
    size_t hash = hm->hash_func(key);
    hash&=hm->cap-1;
    entry_t* tmp;
    while ( !is_tomb(tmp = &hm->data[hash]) && strcmp(key,tmp->key)) {
        hash = (hash+1) & (hm->cap-1);
    }
    res->index = hash;
    res->p = tmp;
}

void hm_hit(hashmap_t* hm,const char* key,queres_t* res) {
    pthread_mutex_lock(&hm->mutex[HM_MUT_REHASH1]);
    hm_hit2(hm,key,res);
    pthread_mutex_unlock(&hm->mutex[HM_MUT_REHASH1]);
}

const char* hm_get(hashmap_t* hm,const char* key) {
    queres_t hit;
    hm_hit(hm,key,&hit);
    return hit.p ? hit.p->data : 0;
}

#ifdef DEBUG
#include "log.h"
    void hm_log_all(hashmap_t* hm) {
        for (unsigned  i = 0; i< hm->cap; i++) {
            if (!is_tomb(hm->data+i)) LOG(DBG,"comp_reg_entry at %u : %s \n flags = %x, index = %u, key = %s",i,hm->data[i].data, hm->data[i].flags, hm->data[i].index, hm->data[i].key);
        }
    }
#endif

void hm_rehash(hashmap_t* hm) {
    pthread_mutex_lock(&hm->mutex[HM_MUT_REHASH2]);
    entry_t* aux = sw_malloc(sizeof(entry_t) * hm->n_entries);
    for (size_t i = 0, o = 0; i<hm->cap; i++) {
        if (!is_tomb(hm->data+i)) memcpy(&aux[o++],&hm->data[i],sizeof(entry_t));
    }
    hm->cap <<=1;
    {
        const size_t nbytes = sizeof(entry_t) * hm->cap;
        hm->data = sw_realloc(hm->data,nbytes);
        memset(hm->data,0,nbytes);
    }
    for (size_t i = 0; i<hm->n_entries; i++) {
        queres_t hit;
        hm_hit2(hm,aux[i].key,&hit);
        *aux[i].index = hit.index;
        memcpy(&hm->data[hit.index],&aux[i],sizeof(entry_t));
    }
    sw_free(aux);
    pthread_mutex_unlock(&hm->mutex[HM_MUT_REHASH2]);
}

size_t* hm_set(hashmap_t* hm,const char* key, void* data, enum HASH_MAP_STORE_FLAGS flags) {
    queres_t hit;
    pthread_mutex_lock(&hm->mutex[HM_MUT_REHASH1]);
    hm_hit2(hm,key,&hit);
    if (!is_tomb(hit.p)) {
        hit.p->data = data;              //modifying existing entry
    } else {
        hm->n_entries++;

        hm->data[hit.index].data = data;
        hm->data[hit.index].key = (flags & HM_STORE_KEYDUP) ? sw_strdup(key) : key;
        hm->data[hit.index].index = sw_malloc(sizeof(size_t));
        *hm->data[hit.index].index = hit.index;
        hm->data[hit.index].flags = flags;
    }

    if (hm->n_entries >= ((hm->cap>>1 )+ (hm->cap>>2)))
        hm_rehash(hm);
    pthread_mutex_unlock(&hm->mutex[HM_MUT_REHASH1]);
    return hm->data[hit.index].index;
}

size_t* hm_get_it(hashmap_t* hm,const char* key) {
    queres_t hit;
    hm_hit(hm,key,&hit);
    return hm->data[hit.index].index;
}