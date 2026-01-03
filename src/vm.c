#ifdef CODEGEN

#ifndef NO_STD_INC
#include <stdio.h>
#include <stdlib.h>
#endif

#ifndef NO_CUSTOM_INC
#include "codegen.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"
#endif

void stack_init(Stack *stack) {
  stack->capacity = 128;
  // stack->bottom = malloc(stack->capacity);
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
int stack_pop(Stack *stack) { return *--stack->top; }

void cyr_vm_init(CyrVM *cyr_vm, DynArr *var_decls, DynArr *codes) {
  cyr_vm->codes = codes;
  cyr_vm->var_decls = var_decls;
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

int load_const(Stack *stack, union OpCodeData dat) {
  stack_push(stack, dat.constant);
  return 1;
}

int load_int(Stack *stack, union OpCodeData dat) {
  stack_push(stack, int_read(dat.ptr));
  return 1;
}

int load_arr(Stack *stack, union OpCodeData dat) {
  int idx = stack_pop(stack);
  stack_push(stack, arr_read(dat.ptr, idx));
  return 1;
}

int store_int(Stack *stack, union OpCodeData dat) {
  int_write(dat.ptr, stack_pop(stack));
  return 1;
}

int store_arr(Stack *stack, union OpCodeData dat) {
  int idx = stack_pop(stack);
  arr_write(dat.ptr, idx, stack_pop(stack));
  return 1;
}

int jmp(Stack *stack, union OpCodeData dat) { return dat.offset; }

// int jz(Stack *stack, union OpCodeData dat) {
//   return stack_pop(stack) ? 1 : dat.offset;
// }

// int jnz(Stack *stack, union OpCodeData dat) {
//   return stack_pop(stack) ? dat.offset : 1;
// }

int jlt(Stack *stack, union OpCodeData dat) {
  stack->top -= 2;
  return *(stack->top + 1) < *stack->top ? dat.offset : 1;
}

int jgt(Stack *stack, union OpCodeData dat) {
  stack->top -= 2;
  return *(stack->top + 1) > *stack->top ? dat.offset : 1;
}

int jeq(Stack *stack, union OpCodeData dat) {
  stack->top -= 2;
  return *(stack->top + 1) == *stack->top ? dat.offset : 1;
}

int jle(Stack *stack, union OpCodeData dat) {
  stack->top -= 2;
  return *(stack->top + 1) <= *stack->top ? dat.offset : 1;
}

int jge(Stack *stack, union OpCodeData dat) {
  stack->top -= 2;
  return *(stack->top + 1) >= *stack->top ? dat.offset : 1;
}

int jneq(Stack *stack, union OpCodeData dat) {
  stack->top -= 2;
  return *(stack->top + 1) != *stack->top ? dat.offset : 1;
}

// int cmp(Stack *stack, union OpCodeData dat) {
//   int *left = --stack->top;
//   int *right = stack->top - 1;
//   *right = do_cmp(dat.cmp_typ, *left, *right);
//   return 1;
// }

int incr(Stack *stack, union OpCodeData dat) {
  *(stack->top - 1) += dat.constant;
  return 1;
}

int binadd(Stack *stack, union OpCodeData dat) {
  stack->top -= 1;
  *(stack->top - 1) += *stack->top;
  return 1;
}

int triadd(Stack *stack, union OpCodeData dat) {
  stack->top -= 2;
  *(stack->top - 1) += *stack->top + *(stack->top + 1);
  return 1;
}

int quadadd(Stack *stack, union OpCodeData dat) {
  stack->top -= 3;
  *(stack->top - 1) += *stack->top + *(stack->top + 1) + *(stack->top + 2);
  return 1;
}

int adds(Stack *stack, union OpCodeData dat) {
  stack->top -= dat.term_cnts;
  int *res = stack->top;
  for (int i = 1; i < dat.term_cnts; i++) {
    *res += stack->top[i];
  }
  stack->top++;
  return 1;
}

int cmul(Stack *stack, union OpCodeData dat) {
  *(stack->top - 1) *= dat.constant;
  return 1;
}

int empty_func(Stack *stack, union OpCodeData dat) { return 1; }

int put(Stack *stack, union OpCodeData dat) {
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
    // [OP_JZ] = jz,
    // [OP_JNZ] = jnz,
    // [OP_CMP] = cmp,
    [OP_JLT] = jlt,
    [OP_JGT] = jgt,
    [OP_JEQ] = jeq,
    [OP_JLE] = jle,
    [OP_JGE] = jge,
    [OP_JNEQ] = jneq,
    [OP_ADDS] = adds,
    [OP_CMUL] = cmul,
    [OP_BINADD] = binadd,
    [OP_TRIADD] = triadd,
    [OP_QUADADD] = quadadd,
    [OP_INCR] = incr,
    [OP_PUT] = put,
    [OP_HALT] = empty_func,
};

int bad_commands(Stack *stack, OpCode *op) {
  // union OpCodeData dat = op->data;
  // switch (op->typ) {
  // // case OP_JMP:
  // //   return jmp(stack, dat);
  // //   break;
  // // // case OP_JZ:
  // // //   return jz(stack, dat);
  // // // case OP_JNZ:
  // // //   return jnz(stack, dat);
  // // // case OP_CMP:
  // // // return cmp(stack, dat);
  // // case OP_JLT:
  // //   return jlt(stack, dat);
  // // case OP_JGT:
  // //   return jgt(stack, dat);
  // // case OP_JEQ:
  // //   return jeq(stack, dat);
  // // case OP_JLE:
  // //   return jle(stack, dat);
  // // case OP_JGE:
  // //   return jge(stack, dat);
  // // case OP_JNEQ:
  // //   return jneq(stack, dat);
  // case OP_ADDS:
  //   return adds(stack, dat);
  // case OP_TRIADD:
  //   return triadd(stack, dat);
  // case OP_QUADADD:
  //   return quadadd(stack, dat);
  // case OP_PUT:
  //   return put(stack, dat);
  // default:
  return handlers[op->typ](stack, op->data);
  // }
}

void cyr_vm_execute(CyrVM *cyr_vm) {
  OpCode *op_ptr = cyr_vm->codes->items;
  Stack stack;
  stack_init(&stack);
  union OpCodeData dat = op_ptr->data;
  int off;
  int cnts = 0;
  int bad_cnts = 0;
  while (1) {
    cnts++;
#ifndef NO_DEBUG
    // printf("%s\n", op_code_type(op_ptr->typ));
#endif
    dat = op_ptr->data;
    switch (op_ptr->typ) {
    case OP_LOAD_CONST:
      off = load_const(&stack, dat);
      break;
    case OP_LOAD_INT:
      off = load_int(&stack, dat);
      break;
    case OP_LOAD_ARR:
      off = load_arr(&stack, dat);
      break;
    case OP_STORE_INT:
      off = store_int(&stack, dat);
      break;
    case OP_STORE_ARR:
      off = store_arr(&stack, dat);
      break;
    case OP_CMUL:
      off = cmul(&stack, dat);
      break;
    case OP_BINADD:
      off = binadd(&stack, dat);
      break;
    case OP_INCR:
      off = incr(&stack, dat);
      break;
    case OP_HALT:
      goto end;
    default:
      bad_cnts++;
      off = bad_commands(&stack, op_ptr);
      break;
    }
    op_ptr += off;
  }
end:
#ifndef NO_DEBUG
  printf("\nDispatched bad commands of %d(%d%%)\n", bad_cnts,
         bad_cnts * 100 / cnts);
  printf("Dispatched good commands of %d(%d%%)\n", cnts - bad_cnts,
         100 - (bad_cnts * 100 / cnts));
  printf("Dispatched commands of %d(100%%)\n", cnts);
#endif
  return;
}

#endif