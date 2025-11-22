
#include "utils.h"

#ifndef NO_STD_INC
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifndef NO_DEBUG
FILE *glob_log_output = NULL;

void set_glob_log_output(FILE *fp) {
  glob_log_output = fp;
  return;
}

int logger(const char *fmt, ...) {
  if (!glob_log_output)
    set_glob_log_output(stderr);

  va_list ap;
  va_start(ap, fmt);
  int res = 0;
  res += vfprintf(glob_log_output, fmt, ap);
  // fflush(glob_log_output);
  va_end(ap);
  return res;
}

void err_log(const char *fmt, ...) {
  if (!glob_log_output)
    set_glob_log_output(stderr);

  va_list ap;
  va_start(ap, fmt);
  fprintf(glob_log_output, "ERR! ");
  vfprintf(glob_log_output, fmt, ap);
  // fflush(glob_log_output);
  va_end(ap);
  exit(-1);
}
#endif

void da_init(DynArr *dyn_arr, size_t item_size, size_t capacity) {
  dyn_arr->item_cnts = 0;
  dyn_arr->capacity = capacity;
  dyn_arr->item_size = item_size;
  dyn_arr->items = malloc(capacity * dyn_arr->item_size);
  // if (dyn_arr->items == NULL) {
  //   err_log("Failed to Allocate %zu Items (in %s)\n", dyn_arr->capacity,
  //           __FUNCTION__);
  // }
}

DynArr *da_create(size_t item_size, size_t capacity) {
  DynArr *dyn_arr = malloc(sizeof(DynArr));
  da_init(dyn_arr, item_size, capacity);
  return dyn_arr;
}

void *da_try_push_back(DynArr *dyn_arr) {
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

void *da_push_back(DynArr *dyn_arr, void *item) {
  void *dest = da_try_push_back(dyn_arr);
  memcpy(dest, item, dyn_arr->item_size);
  return dest;
}

void da_free(DynArr *dyn_arr) { free(dyn_arr->items); }

void str_pool_init(StrPool *pool, size_t mempool_size, size_t capacity) {
  pool->mempool_size = mempool_size;
  pool->mempool = malloc(mempool_size);
  // if (!pool->mempool) {
  //   err_log("Failed to alloc mempool for StrPool (in %s)\n", __FUNCTION__);
  // }
  da_init(&pool->pool_nodes, sizeof(StrPoolNode), capacity);
  return;
}

StrPool *str_pool_create(size_t mempool_size, size_t capacity) {
  StrPool *pool = malloc(sizeof(StrPool));
  str_pool_init(pool, mempool_size, capacity);
  return pool;
}

void str_pool_free(StrPool *str_pool) {
  StrPoolNode *node;
  for (size_t i = 0; i < str_pool->pool_nodes.item_cnts; i++) {
    node = da_get(&str_pool->pool_nodes, i);
    if (!node->alloc_by_pool) {
      free(node->str);
    }
  }
  da_free(&str_pool->pool_nodes);
  free(str_pool->mempool);
  return;
}
char *str_pool_intern(StrPool *str_pool, const char *string, size_t len) {
  StrPoolNode *dest_node = NULL;
  size_t str_node_cnts = str_pool->pool_nodes.item_cnts;
  // Check if exist
  for (size_t i = 0; i < str_node_cnts; i++) {
    dest_node = da_get(&str_pool->pool_nodes, i);
    if (dest_node->len == len && (strncmp(dest_node->str, string, len) == 0)) {
      return dest_node->str;
    }
  }
  // Not exist
  dest_node = da_try_push_back(&str_pool->pool_nodes);
  dest_node->len = len;
  if (len + 1 < str_pool->mempool_size - str_pool->mempool_offset) {
    dest_node->alloc_by_pool = 1;
    dest_node->str = &str_pool->mempool[str_pool->mempool_offset];
    str_pool->mempool_offset += len + 1;
  } else {
    dest_node->alloc_by_pool = 0;
    dest_node->str = malloc(len + 1);
  }
  memcpy(dest_node->str, string, len);
  dest_node->str[len] = '\0';
  return dest_node->str;
}

#ifndef NO_DEBUG
void debug_str_pool(StrPool *str_pool) {
  logger("String Pool:\n");
  StrPoolNode *str_node = NULL;
  size_t str_node_cnts = str_pool->pool_nodes.item_cnts;
  char *node_str = NULL;
  for (size_t i = 0; i < str_node_cnts; i++) {
    str_node = da_get(&str_pool->pool_nodes, i);
    node_str = str_node->str;
    logger("[%zu] = \"%s\" (%zu)\n", i, node_str, str_node->len);
  }
}
#endif
