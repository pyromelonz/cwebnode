#ifndef TREE_FILE_H
#define TREE_FILE_H

#include "rb.h"
#include <stdint.h>
#include "types.h"
#include <stddef.h>

err_t tree_load(const char* path, rbtree* t, size_t data_size);
err_t tree_save(const char* path, rbtree* t, size_t data_size);

#endif //TREE_FILE_H