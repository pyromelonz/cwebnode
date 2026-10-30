#include "tree_misc.h"
#include "log.h"
#include <stdio.h>
#include "misc.h"

#include "config.h"

struct savecookie {
    FILE* file;
    size_t item_size;
};

int tree_save_helper(void* d,void* sizecookie) {
    struct savecookie* c =  (struct savecookie *)sizecookie;
    scramble(d, SCRAMBLEKEY, c->item_size);
    fwrite(d, c->item_size,1,c->file);
    return 0;
}

err_t tree_save(const char* path, rbtree* t, size_t data_size) {

    err_t ret = CRIT;
 	FILE* file = fopen (path,"w");
	if (!file) {
		log_msg(CRIT,"No data saved to file, couldn't open %s",path);
	} else {
		if (RB_ISEMPTY(t)) log_msg(WARN,"Tried to save empty tree"); else {
            struct savecookie c;
            c.file = file;
            c.item_size = data_size;
			RB_APPLY(t, tree_save_helper,&c,INORDER);
			ret = OK;
		}
		fclose (file);
	}
	return ret;
}

err_t tree_load(const char* path, rbtree* t, size_t data_size) {
	char ret = CRIT;

    if (!t) {
        log_msg(CRIT,"No tree to load data into");
        return ret;
    }

	FILE* file = fopen(path,"r");

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