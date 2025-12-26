#ifndef _VM_H_
#define _VM_H_

#include "utils.h"

typedef struct Stack {
  int capacity;
  int *bottom;
  int *top;
} Stack;

typedef struct CyrVM {
  DynArr *var_decls;
  DynArr *codes;
  Stack stack;
} CyrVM;

void cyr_vm_init(CyrVM *cyr_vm, DynArr *var_decls, DynArr *codes);
CyrVM *cyr_vm_create(DynArr *var_decls, DynArr *codes);
void cyr_vm_execute(CyrVM *cyr_vm);

#endif