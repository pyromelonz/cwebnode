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

//SAVING HASH MAP TO FILE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

static int hm_write_exact(int fd, const void* buf, size_t size) {
    const unsigned char* p = buf;
    while (size) {
        ssize_t n = write(fd, p, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { if (!n) errno = EIO; return -1; }
        p += n;
        size -= (size_t)n;
    }
    return 0;
}

static int hm_read_exact(int fd, void* buf, size_t size) {
    unsigned char* p = buf;
    while (size) {
        ssize_t n = read(fd, p, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { if (!n) errno = EIO; return -1; }
        p += n;
        size -= (size_t)n;
    }
    return 0;
}

/* Fixed little-endian lengths, independent of sizeof(size_t). */
static int hm_write_u64(int fd, uint64_t value) {
    unsigned char bytes[8];
    for (unsigned i = 0; i < 8; i++) bytes[i] = (unsigned char)(value >> (8 * i));
    return hm_write_exact(fd, bytes, sizeof(bytes));
}

static int hm_read_u64(int fd, uint64_t* value) {
    unsigned char bytes[8];
    if (hm_read_exact(fd, bytes, sizeof(bytes)) < 0) return -1;
    *value = 0;
    for (unsigned i = 0; i < 8; i++) *value |= (uint64_t)bytes[i] << (8 * i);
    return 0;
}

int hm_save(const char* path, hashmap_t* hm,
            int (*data_write)(void* data, int fd)) {
    static const char magic[4] = {'H', 'M', '0', '1'};
    if (!path || !hm || !data_write) { errno = EINVAL; return -1; }

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return -1;

    int result = -1;
    uint64_t count = 0;

    // Count occupied slots, independent of flags and n_entries
    for (size_t i = 0; i < hm->cap; i++)
        if (!is_tomb(&hm->data[i])) count++;

    if (hm_write_exact(fd, magic, sizeof(magic)) < 0 ||
        hm_write_u64(fd, count) < 0) goto done;

    for (size_t i = 0; i < hm->cap; i++) {
        entry_t* e = &hm->data[i];
        if (is_tomb(e)) continue;

        size_t key_len = strlen(e->key);
        if (hm_write_u64(fd, (uint64_t)key_len) < 0 ||
            hm_write_exact(fd, e->key, key_len) < 0) goto done;

        // Reserve payload length, then backfill after writing
        off_t len_pos = lseek(fd, 0, SEEK_CUR);
        if (len_pos < 0 || hm_write_u64(fd, 0) < 0) goto done;

        off_t begin = lseek(fd, 0, SEEK_CUR);
        if (begin < 0) goto done;

        int written = data_write(e->data, fd);
        off_t end = lseek(fd, 0, SEEK_CUR);

        if (written < 0 || end < begin || end - begin != written) {
            errno = EIO;
            goto done;
        }

        if (lseek(fd, len_pos, SEEK_SET) < 0 ||
            hm_write_u64(fd, (uint64_t)written) < 0 ||
            lseek(fd, end, SEEK_SET) < 0) goto done;
    }

    result = 0;

done:
    if (close(fd) < 0) result = -1;
    return result;
}


int hm_load(const char* path, hashmap_t* hm,
            void* (*data_read)(int fd)) {
    static const char magic[4] = {'H', 'M', '0', '1'};
    if (!path || !hm || !data_read) { errno = EINVAL; return -1; }

    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    int result = -1;
    hashmap_t* fresh = new_hm();
    if (!fresh) {
        close(fd);
        errno = ENOMEM;
        return -1;
    }
    fresh->hash_func = hm->hash_func;

    char header[4];
    uint64_t count;

    if (hm_read_exact(fd, header, sizeof(header)) < 0 ||
        memcmp(header, magic, sizeof(header)) != 0 ||
        hm_read_u64(fd, &count) < 0) {
        errno = EINVAL;
        goto done;
    }

    for (uint64_t i = 0; i < count; i++) {
        uint64_t key_len, value_len;
        char* key = NULL;
        void* value = NULL;

        if (hm_read_u64(fd, &key_len) < 0 ||
            key_len > SIZE_MAX - 1) {
            errno = EINVAL;
            goto fail_entry;
        }

        key = sw_malloc((size_t)key_len + 1);
        if (!key) {
            errno = ENOMEM;
            goto fail_entry;
        }

        if (hm_read_exact(fd, key, (size_t)key_len) < 0)
            goto fail_entry;

        key[key_len] = '\0';

        if (memchr(key, '\0', (size_t)key_len)) {
            errno = EINVAL;
            goto fail_entry;
        }

        queres_t hit;
        hm_hit2(fresh, key, &hit);
        if (!is_tomb(hit.p)) {
            errno = EINVAL;
            goto fail_entry;
        }

        if (hm_read_u64(fd, &value_len) < 0 ||
            value_len > INT_MAX) {
            errno = EINVAL;
            goto fail_entry;
        }

        off_t begin = lseek(fd, 0, SEEK_CUR);
        if (begin < 0) goto fail_entry;

        // Callback allocates and returns the restored value
        value = data_read(fd);

        off_t end = lseek(fd, 0, SEEK_CUR);
        if (!value || end < begin ||
            (uint64_t)(end - begin) != value_len) {
            errno = EIO;
            goto fail_entry;
        }

        hm_set(fresh, key, value,
               HM_STORE_KEYDUP | HM_STORE_FREEDATA);
        sw_free(key);
        continue;

fail_entry:
        sw_free(key);
        sw_free(value);
        goto done;
    }

    if (close(fd) < 0) {
        fd = -1;
        goto done;
    }
    fd = -1;

    // Replace contents, preserving existing mutexes and hash function
    entry_t* old_data = hm->data;
    size_t old_cap = hm->cap;
    size_t old_count = hm->n_entries;

    hm->data = fresh->data;
    hm->cap = fresh->cap;
    hm->n_entries = fresh->n_entries;

    fresh->data = old_data;
    fresh->cap = old_cap;
    fresh->n_entries = old_count;

    result = 0;

done:
    for (int i = 0; i < N_HM_MUTEXES; i++)
        pthread_mutex_destroy(&fresh->mutex[i]);

    delete_hm(fresh);
    if (fd >= 0 && close(fd) < 0) result = -1;
    return result;
}