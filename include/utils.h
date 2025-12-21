#ifndef _UTILS_H_
#define _UTILS_H_

#pragma once

#ifndef NO_STD_INC
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#endif

typedef struct DynArr {
  void *items;
  size_t item_size;
  size_t item_cnts; // Index shouldn't exceed it.
  size_t capacity;  // How many items should allocate.
} DynArr;

#ifndef NO_DEBUG
void set_glob_log_output(FILE *fp);
int logger(const char *fmt, ...);
void err_log(const char *fmt, ...);
#endif

void da_init(DynArr *dyn_arr, size_t item_size, size_t capacity);
DynArr *da_create(size_t item_size, size_t capacity);
void *da_try_push_back(DynArr *dyn_arr);
void *da_push_back(DynArr *dyn_arr, void *item);
static inline void *da_get(DynArr *dyn_arr, size_t idx) {
  // if (idx >= dyn_arr->item_cnts) {
  //   err_log("Out of Bounds (in %s)\n", __FUNCTION__);
  // }
  void *item = dyn_arr->items + idx * dyn_arr->item_size;
  return item;
}
static inline void da_set(DynArr *dyn_arr, size_t idx, void *item) {
  // if (idx >= dyn_arr->item_cnts) {
  //   err_log("Out of Bounds (in %s)\n", __FUNCTION__);
  // }
  void *dest = dyn_arr->items + idx * dyn_arr->item_size;
  memcpy(dest, item, dyn_arr->item_size);
  return;
}
void da_free(DynArr *dyn_arr);

typedef struct StrPoolNode {
  char *str;
  size_t len;
  char alloc_by_pool;
} StrPoolNode;

typedef struct StrPool {
  // char *pool_space; // A space stored all of the string
  // size_t length;    // length of `pool_space`
  // size_t capacity;
  char *mempool;
  size_t mempool_offset;
  size_t mempool_size;
  DynArr pool_nodes; // DynArr that store some StrPoolNode
} StrPool;

void str_pool_init(StrPool *pool, size_t mempool_size, size_t capacity);
StrPool *str_pool_create(size_t mempool_size, size_t capacity);
void str_pool_free(StrPool *str_pool);
char *str_pool_intern(StrPool *str_pool, const char *string, size_t len);
#ifndef NO_DEBUG
void debug_str_pool(StrPool *str_pool);
#endif

#endif
