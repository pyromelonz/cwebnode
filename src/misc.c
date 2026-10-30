
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "log.h"

#include <stdarg.h>
#include <stdio.h>

char* file2str(const char* path) {
    char buf[32];
    unsigned size = 0;
    FILE* f = fopen(path,"r");
    int read;
    while (read = fread(buf,1,32,f)) size+= read;
    rewind(f);
    char* str = sw_malloc(size+1);
    if (fread(str,1,size,f) < size) {
        LOG(WARN,"Couldn't read entire file from path %s",path);
    }
    str[size] = 0;
    fclose(f);
    return str;
}

//format must be "key key key" anything else is undefined
//allocates strings of post values into char** parameters after and corresponding to format
unsigned read_post_data(const char* post_data, const char* fmt, ...) {
    if (!(post_data && *post_data && fmt && *fmt))
    return 0;

    
    va_list args;
    unsigned nargs = 0, n_read = 0;
    
    const char* s1 = fmt;

    while (*s1) {
        nargs++;
        s1 = strchrnul(s1, ' ');
        s1 += !!*s1;
    };

    char** keys = sw_malloc(sizeof(char *) * nargs);

    s1 = fmt;

    {
        unsigned i = 0, keylen;
        char* t;
        do {
            t = strchrnul(s1, ' ');
            keylen = (t-s1);
            keys[i] = sw_malloc(keylen+1);
            strncpy(keys[i],s1,keylen);
            keys[i++][keylen]=0;
            s1=t+(!!*t);
        } while (*s1);
    }
    
    char* pd = sw_strdup(post_data), *s = pd, *r = strtok(s, "&");

    while (s) {
        char *m = strchr(s,'=');
        if (m) {
            *m=0;
            va_start(args, fmt);
            for (int i = 0; i< nargs; i++) {
                char** p = va_arg(args, char**);
                if (!strcmp(s,keys[i])) {
                    *p = sw_strdup(++m);
					n_read++;
                }
            }
        }
        s = r;
        r = strtok(0,"&");
    }

    while (nargs--) {
        sw_free(keys[nargs]);
    }
    sw_free(keys);
    sw_free(pd);

    va_end(args);
	return n_read;
}

#define SCRAMBLE_PASSES 4


//not really secure, not much more than obfuscation

void scramble(void* a, uint32_t key, uint32_t bytes) {
	for (int i = 0; i<(bytes>>2); i++) {
		uint32_t* u = ((uint32_t*)a)+i;
		for (int i = 0; i<SCRAMBLE_PASSES; i++) {
 			*u ^= key;
			*u = (*u << i) | (*u >> (32 - i));
		}
	}

    if (bytes & 3) {
        uint8_t* u = ((uint8_t*)a)+(bytes & ~3);
        for (int i = 0; i<SCRAMBLE_PASSES; i++) {
            *u ^= (key >> (i*8)) & 0xFF;
            *u = (*u << i) | (*u >> (8 - i));
        }
    }
}

void unscramble(void* a, uint32_t key, uint32_t bytes) {
	for (int i = 0; i<(bytes>>2); i++) {
		uint32_t* u = ((uint32_t*)a)+i;
		for (int i = SCRAMBLE_PASSES; i--;) {
			*u = (*u << (32- i)) | (*u >> i);
 			*u ^= key;
		}
	}

    if (bytes & 3) {
        uint8_t* u = ((uint8_t*)a)+(bytes & ~3);
        for (int i = SCRAMBLE_PASSES; i--;) {
            *u = (*u << (8 - i)) | (*u >> i);
            *u ^= (key >> (i*8)) & 0xFF;
        }
    }
}

arena_t* new_arena(size_t cap) {
    arena_t* a = sw_malloc(sizeof(arena_t));
    a->data = sw_malloc(cap);
    a->size = cap;
    a->it = a->data;
}

void delete_arena(arena_t* a) {
    sw_free(a->data);
    sw_free(a);
}

void* arena_malloc(arena_t* a, size_t size) {
    void* ret = 0;
    if (((char*)a->it + size) < ((char*)a->data+a->size)) {
        //enough space
        ret= a->it;
        a->it=((char*)a->it) + size;
    }
    return ret;
}