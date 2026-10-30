#include "wn_component_registry.h"
#include <pthread.h>
#include "murmur3.h"
#include "mem_dbg.h"
#include <string.h>
#include "limits.h"

#define X(e,s) [REG_##e] = s,
const char* static_comp_reg[_STATIC_WWW_COMPONENTS_COUNT] = {_STATIC_WWW_COMPONENTS_LIST};
#undef X

// HASHMAP


typedef struct {
    size_t n_entries, cap;
    size_t (*hash_func)(const char* key);
    entry_t** data;   //declare this extern
} hashmap_t;

static hashmap_t comp_reg;

void* comp_reg_data(size_t index) {
    return comp_reg.data[index]->data;
}

size_t comp_reg_max_index() {
    return comp_reg.n_entries+_STATIC_WWW_COMPONENTS_EXTRA;
}

size_t comp_reg_hash(const char* key) {
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

void comp_reg_init() {
    comp_reg.hash_func = comp_reg_hash;
    comp_reg.data = sw_calloc(16,sizeof(entry_t*));
    comp_reg.n_entries = 0;
    comp_reg.cap = 16;
}

static void nop(void*){}

static p_sw_free(compreg)
static void (*comp_reg_freers[2])(void*) = {nop,compreg_sw_free};

void comp_reg_term() {
    for (size_t i = 0; i<comp_reg.cap; i++) {
        entry_t* o = comp_reg.data[i];
        if (o) {
            comp_reg_freers[!!(o->flags & CRSTORE_FREEDATA)](o->data);
            if (CRSTORE_KEYDUP & o->flags) sw_free(o->key);
            sw_free(o);
        }
    }
    sw_free(comp_reg.data);
}

pthread_mutex_t data_mut = PTHREAD_MUTEX_INITIALIZER, rehash1 = PTHREAD_MUTEX_INITIALIZER, rehash2 = PTHREAD_MUTEX_INITIALIZER, blocker = PTHREAD_MUTEX_INITIALIZER;


static unsigned blockers = 0;

//DO NOT ASSIGN NEW ENTRIES DURING BLOCK!!!!
void cr_block_rehash() {
    pthread_mutex_lock(&blocker);
    if (!blockers++) pthread_mutex_lock(&rehash2);
    pthread_mutex_unlock(&blocker);
}

//DO NOT ASSIGN NEW ENTRIES DURING BLOCK!!!!
void cr_unblock_rehash() {
    pthread_mutex_lock(&blocker);
    blockers = (!!blockers) * (blockers-1);
    if (!blockers) pthread_mutex_unlock(&rehash2);
    pthread_mutex_unlock(&blocker);
}

typedef struct {
    size_t index;
    entry_t* p;
} queres_t;

void cr_hit2(const char* key,queres_t* res) {
    size_t hash = comp_reg.hash_func(key);
    hash&=comp_reg.cap-1;
    entry_t* tmp;
    while ( (tmp = comp_reg.data[hash]) && strcmp(key,tmp->key)) {
        hash = (hash+1) & (comp_reg.cap-1);
    }
    res->index = hash;
    res->p = tmp;
}

void cr_hit(const char* key,queres_t* res) {
    pthread_mutex_lock(&rehash1);
    cr_hit2(key,res);
    pthread_mutex_unlock(&rehash1);
}

const char* comp_reg_get(const char* key) {
    queres_t hit;
    cr_hit(key,&hit);
    return hit.p ? hit.p->data : 0;
}

const char* comp_reg_ambi_get(reg_ambindex i) {
    /* works too but branches
    switch ((i < _STATIC_WWW_COMPONENTS_COUNT) - (i == _STATIC_WWW_COMPONENTS_EXTRA)) {
        case 1: return static_comp_reg[i];
        case -1: return comp_reg_data(i- _STATIC_WWW_COMPONENTS_COUNT);
        default: return 0;
    }
    return 0; */
    return (const char*)(((intptr_t)static_comp_reg[i] * (i < _STATIC_WWW_COMPONENTS_COUNT)) | ((intptr_t)comp_reg.data[i - _STATIC_WWW_COMPONENTS_EXTRA]->data * (i > _STATIC_WWW_COMPONENTS_COUNT)));
}

#ifdef DEBUG
#include "log.h"
    void comp_reg_log_all() {
        for (unsigned  i = 0; i< comp_reg.cap; i++) {
            if (comp_reg.data[i]) LOG(DBG,"comp_reg_entry at %u : %s \n flags = %x, index = %u, key = %s",i,comp_reg.data[i]->data, comp_reg.data[i]->flags, comp_reg.data[i]->index, comp_reg.data[i]->key);
        }
    }
#endif

void comp_reg_rehash() {
    pthread_mutex_lock(&rehash2);
    entry_t** aux = sw_malloc(sizeof(entry_t*) * comp_reg.n_entries);
    for (size_t i = 0, o = 0; i<comp_reg.cap; i++) {
        if (comp_reg.data[i]) aux[o++] = comp_reg.data[i];
    }
    comp_reg.cap <<=1;
    {
        const size_t nbytes = sizeof(entry_t*) * comp_reg.cap;
        comp_reg.data = sw_realloc(comp_reg.data,nbytes);
        memset(comp_reg.data,0,nbytes);
    }
    for (size_t i = 0; i<comp_reg.n_entries; i++) {
        queres_t hit;
        cr_hit2(aux[i]->key,&hit);
        comp_reg.data[hit.index] = aux[i];
        comp_reg.data[hit.index]->index = hit.index;
    }
    sw_free(aux);
    pthread_mutex_unlock(&rehash2);
}

entry_t* comp_reg_set(const char* key, void* data, enum COMP_REG_STORE_FLAGS flags) {
    queres_t hit;
    pthread_mutex_lock(&rehash1);
    cr_hit2(key,&hit);
    if (hit.p) {
        hit.p->data = data;                   //modifying existing entry
    } else {
        comp_reg.n_entries++;
        hit.p = sw_malloc(sizeof(entry_t));   //new entry
        hit.p->data = data;
        hit.p->key = (flags & CRSTORE_KEYDUP) ? sw_strdup(key) : key;
        hit.p->index = hit.index;
        hit.p->flags = flags;
        comp_reg.data[hit.index] = hit.p;
    }

    if (comp_reg.n_entries >= ((comp_reg.cap>>1 )+ (comp_reg.cap>>2)))
        comp_reg_rehash();
    pthread_mutex_unlock(&rehash1);
    return hit.p;
}

entry_t* comp_reg_get_p(const char* key) {
    queres_t hit;
    cr_hit(key,&hit);
    return hit.p;
}