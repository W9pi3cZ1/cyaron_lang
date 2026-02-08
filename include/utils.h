#ifndef _UTILS_H_
#define _UTILS_H_

#pragma once

#ifndef NO_STD_INC
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#endif
#ifndef NO_DEBUG
#ifndef NO_STD_INC
#include <stdio.h>
#endif
#endif

extern int da_init_call_cnts;
typedef unsigned int usize;

typedef struct DynArr {
  void *items;
  usize item_size;
  usize item_cnts; // Index shouldn't exceed it.
  usize capacity;  // How many items should allocate.
} DynArr;

#ifndef NO_DEBUG
void set_glob_log_output(FILE *fp);
int logger(const char *fmt, ...);
void err_log(const char *fmt, ...);
#endif

void da_init(DynArr *dyn_arr, usize item_size, usize capacity);
DynArr *da_create(usize item_size, usize capacity);
// void *da_try_push_back(DynArr *dyn_arr);
static inline void *da_try_push_back(DynArr *dyn_arr) {
  if (dyn_arr->item_cnts >= dyn_arr->capacity) {
    dyn_arr->capacity *= 2; // 2x Extend
    dyn_arr->items =
        realloc(dyn_arr->items, dyn_arr->capacity * dyn_arr->item_size);
    // if (dyn_arr->items == NULL) {
    //   err_log("Failed to Re-Allocate %zu Items (in %s)\n", dyn_arr->capacity,
    //           __FUNCTION__);
    // }
  }
  void *dest = dyn_arr->items + dyn_arr->item_cnts * dyn_arr->item_size;
  ++dyn_arr->item_cnts;
  return dest;
}
void *da_push_back(DynArr *dyn_arr, void *item);
static inline void *da_get(DynArr *dyn_arr, usize idx) {
  // if (idx >= dyn_arr->item_cnts) {
  //   err_log("Out of Bounds (in %s)\n", __FUNCTION__);
  // }
  void *item = dyn_arr->items + idx * dyn_arr->item_size;
  return item;
}
static inline void da_set(DynArr *dyn_arr, usize idx, void *item) {
  // if (idx >= dyn_arr->item_cnts) {
  //   err_log("Out of Bounds (in %s)\n", __FUNCTION__);
  // }
  void *dest = dyn_arr->items + idx * dyn_arr->item_size;
  memcpy(dest, item, dyn_arr->item_size);
  return;
}
static inline void *da_pushs_back(DynArr *dyn_arr, usize cnts) {
  if (dyn_arr->item_cnts + cnts >= dyn_arr->capacity) {
    while (dyn_arr->capacity > dyn_arr->item_cnts + cnts) { // loop
      dyn_arr->capacity *= 2;                               // 2x Extend
    }
    dyn_arr->items =
        realloc(dyn_arr->items, dyn_arr->capacity * dyn_arr->item_size);
    // if (dyn_arr->items == NULL) {
    //   err_log("Failed to Re-Allocate %zu Items (in %s)\n", dyn_arr->capacity,
    //           __FUNCTION__);
    // }
  }
  void *dest = dyn_arr->items + dyn_arr->item_cnts * dyn_arr->item_size;
  dyn_arr->item_cnts += cnts;
  return dest;
}
void da_free(DynArr *dyn_arr);

typedef struct StrPoolNode {
  char *str;
  usize len;
  char alloc_by_pool;
} StrPoolNode;

typedef struct StrPool {
  // char *pool_space; // A space stored all of the string
  // usize length;    // length of `pool_space`
  // usize capacity;
  char *mempool;
  usize mempool_offset;
  usize mempool_size;
  DynArr pool_nodes; // DynArr that store some StrPoolNode
} StrPool;

static inline int lite_strncmp(const char *s1, const char *s2, usize n) {
  // from musl libc
  const unsigned char *l = (void *)s1, *r = (void *)s2;
  if (!n--)
    return 0;
  for (; *l && *r && n && *l == *r; l++, r++, n--)
    ;
  return *l - *r;
}
void str_pool_init(StrPool *pool, usize mempool_size, usize capacity);
StrPool *str_pool_create(usize mempool_size, usize capacity);
void str_pool_free(StrPool *str_pool);
char *str_pool_intern(StrPool *str_pool, const char *string, usize len);
#ifndef NO_DEBUG
void debug_str_pool(StrPool *str_pool);
#endif

#endif
