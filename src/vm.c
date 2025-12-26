#include "vm.h"
#include "codegen.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

void stack_init(Stack *stack) {
  stack->capacity = 256;
  stack->bottom = malloc(stack->capacity);
  stack->top = stack->bottom;
}

void stack_check_capacity(Stack *stack) {
  if (stack->top >= stack->bottom + stack->capacity) {
    printf("Stack Overflow!");
    // int diff = stack->top - stack->bottom;
    // stack->capacity *= 2;
    // stack->bottom = realloc(stack->bottom, stack->capacity);
    // stack->top = stack->bottom + diff;
  }
}

void stack_push(Stack *stack, int data) {
  // stack_check_capacity(stack);
  *stack->top++ = data;
}
int stack_pop(Stack *stack) {
  stack->top--;
  return *stack->top;
}

void cyr_vm_init(CyrVM *cyr_vm, DynArr *var_decls, DynArr *codes) {
  cyr_vm->codes = codes;
  cyr_vm->var_decls = var_decls;
  stack_init(&cyr_vm->stack);
}

CyrVM *cyr_vm_create(DynArr *var_decls, DynArr *codes) {
  CyrVM *cyr_vm = malloc(sizeof(CyrVM));
  cyr_vm_init(cyr_vm, var_decls, codes);
  return cyr_vm;
}

int arr_read(VarDecl *decl, int idx) {
  return decl->data.a.arr[idx - decl->start];
}

int int_read(VarDecl *decl) { return decl->data.i.val; }

void arr_write(VarDecl *decl, int idx, int val) {
  decl->data.a.arr[idx - decl->start] = val;
}

void int_write(VarDecl *decl, int val) { decl->data.i.val = val; }

typedef int (*vm_handler)(Stack *stack, union OpCodeData dat);

static int load_const(Stack *stack, union OpCodeData dat) {
  stack_push(stack, dat.constant);
  return 1;
}

static int load_int(Stack *stack, union OpCodeData dat) {
  stack_push(stack, int_read(dat.ptr));
  return 1;
}

static int load_arr(Stack *stack, union OpCodeData dat) {
  int idx = stack_pop(stack);
  stack_push(stack, arr_read(dat.ptr, idx));
  return 1;
}

static int store_int(Stack *stack, union OpCodeData dat) {
  int_write(dat.ptr, stack_pop(stack));
  return 1;
}

static int store_arr(Stack *stack, union OpCodeData dat) {
  int idx = stack_pop(stack);
  arr_write(dat.ptr, idx, stack_pop(stack));
  return 1;
}

static int jmp(Stack *stack, union OpCodeData dat) { return dat.offset; }

static int jz(Stack *stack, union OpCodeData dat) {
  return stack_pop(stack) ? 1 : dat.offset;
}

static int jnz(Stack *stack, union OpCodeData dat) {
  return stack_pop(stack) ? dat.offset : 1;
}

static int cmp(Stack *stack, union OpCodeData dat) {
  int left = stack_pop(stack);
  int right = stack_pop(stack);
  stack_push(stack, do_cmp(dat.cmp_typ, left, right));
  return 1;
}

static int incr(Stack *stack, union OpCodeData dat) {
  ++*(stack->top - 1);
  return 1;
}

static int adds(Stack *stack, union OpCodeData dat) {
  int res = 0;
  for (int i = 0; i < dat.term_cnts; i++) {
    res += stack_pop(stack);
  }
  stack_push(stack, res);
  return 1;
}

static int cmul(Stack *stack, union OpCodeData dat) {
  stack_push(stack, dat.constant * stack_pop(stack));
  return 1;
}

static int empty_func(Stack *stack, union OpCodeData dat) { return 0; }

static int put(Stack *stack, union OpCodeData dat) {
  printf("%d ", stack_pop(stack));
  return 1;
}

static vm_handler handlers[] = {
    [OP_LOAD_CONST] = load_const,
    [OP_LOAD_INT] = load_int,
    [OP_LOAD_ARR] = load_arr,
    [OP_STORE_INT] = store_int,
    [OP_STORE_ARR] = store_arr,
    [OP_JMP] = jmp,
    [OP_JZ] = jz,
    [OP_JNZ] = jnz,
    [OP_CMP] = cmp,
    [OP_INCR] = incr,
    [OP_ADDS] = adds,
    [OP_CMUL] = cmul,
    [OP_PUT] = put,
    [OP_HALT] = empty_func,
};

void cyr_vm_execute(CyrVM *cyr_vm) {
  OpCode *op_ptr = cyr_vm->codes->items;
  Stack *stack = &cyr_vm->stack;
  union OpCodeData dat = op_ptr->data;
  while (op_ptr->typ != OP_HALT) {
#ifndef NO_DEBUG
    // printf("%s\n", op_code_type(op_ptr->typ));
#endif
    dat = op_ptr->data;
    int off;
    off = handlers[op_ptr->typ](stack, dat);
    op_ptr += off;
  }
}