#ifdef CODEGEN

#ifndef _VM_H_
#define _VM_H_

#ifndef NO_CUSTOM_INC
#include "codegen.h"
#include "utils.h"
#endif

typedef struct Stack {
  int *top;
  int bottom[512];
  // int *bottom;
  int capacity;
} Stack;

typedef struct CyrVM {
  DynArr *codes;
  // Stack stack;
  DynArr *var_decls;
} CyrVM;

int get_stack_delta(enum OpCodeType typ);
void cyr_vm_init(CyrVM *cyr_vm, DynArr *var_decls, DynArr *codes);
CyrVM *cyr_vm_create(DynArr *var_decls, DynArr *codes);
void cyr_vm_execute(CyrVM *cyr_vm);

#endif

#endif