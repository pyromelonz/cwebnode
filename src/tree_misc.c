#include "tree_misc.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include "misc.h"

#include "config.h"

struct savecookie {
    FILE* file;
    size_t item_size;
};

int tree_save_helper(void* d,void* sizecookie) {
    struct savecookie* c = (struct savecookie *)sizecookie;

    /* Never scramble tree-owned data in place: it may contain live pointers. */
    void* copy = sw_malloc(c->item_size);
    if (!copy) return 1;

    memcpy(copy, d, c->item_size);
    scramble(copy, SCRAMBLEKEY, c->item_size);
    int write_failed = fwrite(copy, c->item_size, 1, c->file) != 1;
    sw_free(copy);
    return write_failed;
}

err_t tree_save(const char* path, rbtree* t, size_t data_size) {
    err_t ret = CRIT;
    FILE* file = fopen(path, "wb");
    if (!file) {
        log_msg(CRIT, "No data saved to file, couldn't open %s", path);
        return ret;
    }

    if (RB_ISEMPTY(t)) {
        log_msg(WARN, "Tried to save empty tree");
    } else {
        struct savecookie c = { file, data_size };
        if (RB_APPLY(t, tree_save_helper, &c, INORDER) == 0)
            ret = OK;
        else
            log_msg(CRIT, "Failed writing tree to %s", path);
    }

    if (fclose(file) != 0) {
        log_msg(CRIT, "Failed closing tree file %s", path);
        ret = CRIT;
    }
    return ret;
}

err_t tree_load(const char* path, rbtree* t, size_t data_size) {
	char ret = CRIT;

    if (!t) {
        log_msg(CRIT,"No tree to load data into");
        return ret;
    }

	FILE* file = fopen(path,"rb");

	if (file == NULL) {
		log_msg(CRIT,"No file data loaded - couldn't open %s",path);
	} else {

		void* buffer = sw_malloc(data_size);
		while( data_size == fread(buffer,1,data_size,file) ){
			unscramble(buffer, SCRAMBLEKEY, data_size);
			rbnode* tmp = rb_insert(t, buffer);
			buffer = sw_malloc(data_size);
		}
		ret=OK;
		sw_free(buffer);
		fclose(file);
	}
	return ret;
}