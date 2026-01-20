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
  int capacity = sizeof(stack->bottom) / sizeof(*stack->bottom);
  stack->capacity = capacity;
  // stack->bottom = malloc(stack->capacity);
  stack->top = stack->bottom + capacity - 1;
}

void stack_check_capacity(
    Stack *stack) { // 太过神奇了，没有这个“无用函数”反而速度会变慢
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
  *stack->top-- = data;
}
int stack_pop(Stack *stack) {
  // stack_check_capacity(stack);
  return *++stack->top;
}

void cyr_vm_init(CyrVM *cyr_vm, DynArr *var_decls, DynArr *codes) {
  cyr_vm->codes = codes;
  cyr_vm->var_decls = var_decls;
}

CyrVM *cyr_vm_create(DynArr *var_decls, DynArr *codes) {
  CyrVM *cyr_vm = malloc(sizeof(CyrVM));
  cyr_vm_init(cyr_vm, var_decls, codes);
  return cyr_vm;
}

int *arr_ref(VarDecl *decl, int idx) {
  // VarDecl *decl = decl_idx + (VarDecl *)vm->var_decls->items;
  return &decl->data.a.arr[idx - decl->start];
}

int *int_ref(VarDecl *decl) {
  // VarDecl *decl = decl_idx + (VarDecl *)vm->var_decls->items;
  return &decl->data.i.val;
}

typedef short (*vm_handler)(CyrVM *vm, Stack *stack, union OpCodeData dat);
#define DECL_VM_HANDLE(x) short x(CyrVM *vm, Stack *stack, union OpCodeData dat)

DECL_VM_HANDLE(load_const) {
  stack->top[1] = dat.constant;
  return 1;
}

DECL_VM_HANDLE(load_int) {
  // stack->top[1] = *int_ref(vm, dat.decl_idx);
  stack->top[1] = *int_ref(dat.ptr);
  return 1;
}

DECL_VM_HANDLE(load_arr) {
  int idx = stack->top[1];
  // stack->top[1] = *arr_ref(vm, dat.decl_idx, idx);
  stack->top[1] = *arr_ref(dat.ptr, idx);
  return 1;
}

DECL_VM_HANDLE(store_int) {
  // *int_ref(vm, dat.decl_idx) = stack->top[0];
  *int_ref(dat.ptr) = stack->top[0];
  return 1;
}

DECL_VM_HANDLE(store_arr) {
  int idx = stack->top[-1];
  // *arr_ref(vm, dat.decl_idx, idx) = stack->top[0];
  *arr_ref(dat.ptr, idx) = stack->top[0];
  return 1;
}

DECL_VM_HANDLE(seti) {
  // *int_ref(vm, dat.var_const.decl_idx) = dat.var_const.constant;
  *int_ref(dat.var_const.ptr) = dat.var_const.constant;
  return 1;
}

DECL_VM_HANDLE(jmp) { return dat.offset; }

char do_cmp(enum CmpType cond_typ, int left, int right) {
  // Assume it always within range ...
  // if (cond_typ < 0 || cond_typ >= 6)
  //   return 0;

  // enum CmpType {
  //   CMP_LT = 0b001,
  //   CMP_EQ = 0b010,
  //   CMP_LE = 0b011,
  //   CMP_GT = 0b100,
  //   CMP_NEQ = 0b101,
  //   CMP_GE = 0b110,
  // };

  const char gt = left > right;
  const char ge = left >= right;
  return (cond_typ >> (gt + ge)) & 1;
};

DECL_VM_HANDLE(cjmp) {
  int left = stack->top[-1];
  int right = stack->top[0];

  return do_cmp(dat.cjmp.cmp_typ, left, right) ? dat.cjmp.offset : 1;
}

// short cmp(Stack *stack, union OpCodeData dat) {
//   int *left = --stack->top;
//   int *right = stack->top - 1;
//   *right = do_cmp(dat.cmp_typ, *left, *right);
//   return 1;
// }

DECL_VM_HANDLE(incr) {
  stack->top[1] += dat.constant;
  return 1;
}

DECL_VM_HANDLE(inci) {
  // *int_ref(vm, dat.var_const.decl_idx) += dat.var_const.constant;
  *int_ref(dat.var_const.ptr) += dat.var_const.constant;
  return 1;
}

DECL_VM_HANDLE(binadd) {
  stack->top[1] += stack->top[0];
  return 1;
}

// DECL_VM_HANDLE(triadd) {
//   stack->top[1] += stack->top[0] + stack->top[-1];
//   return 1;
// }

// DECL_VM_HANDLE(quadadd) {
//   stack->top[1] += stack->top[0] + stack->top[-1] + stack->top[-2];
//   return 1;
// }

// DECL_VM_HANDLE(adds) {
//   stack->top++;
//   int *res = stack->top;
//   for (int i = 1; i < dat.term_cnts; i++) {
//     *res += stack->top[i];
//   }
//   stack->top += dat.term_cnts - 2;
//   return 1;
// }

DECL_VM_HANDLE(cmul) {
  stack->top[1] *= dat.constant;
  return 1;
}

DECL_VM_HANDLE(empty_func) { return 1; }

DECL_VM_HANDLE(put) {
  printf("%d ", stack->top[0]);
  return 1;
}

// static vm_handler handlers[] = {
//     [OP_LOAD_CONST] = load_const,
//     [OP_LOAD_INT] = load_int,
//     [OP_LOAD_ARR] = load_arr,
//     [OP_STORE_INT] = store_int,
//     [OP_STORE_ARR] = store_arr,
//     [OP_SETI] = seti,
//     [OP_INCR] = incr,
//     [OP_INCI] = inci,
//     [OP_CMUL] = cmul,
//     [OP_BINADD] = binadd,
//     [OP_PUT] = put,
//     [OP_JMP] = jmp,
//     [OP_CJMP] = cjmp,
//     [OP_HALT] = empty_func,
//     // [OP_TRIADD] = triadd,
//     // [OP_QUADADD] = quadadd,
//     // [OP_ADDS] = adds,
// };

#define ST_PO(x) (0 + (x))

static int stack_deltas[] = {
    [OP_LOAD_CONST] = ST_PO(-1), [OP_LOAD_INT] = ST_PO(-1),
    [OP_LOAD_ARR] = ST_PO(0),    [OP_STORE_INT] = ST_PO(1),
    [OP_STORE_ARR] = ST_PO(2),   [OP_SETI] = ST_PO(0),
    [OP_INCR] = ST_PO(0),        [OP_INCI] = ST_PO(0),
    [OP_CMUL] = ST_PO(0),        [OP_BINADD] = ST_PO(1),
    [OP_PUT] = ST_PO(1),         [OP_JMP] = ST_PO(0),
    [OP_CJMP] = ST_PO(2),        [OP_HALT] = ST_PO(0),
    // [OP_TRIADD] = ST_PO(2),      [OP_QUADADD] = ST_PO(3),
    // [OP_ADDS] = ST_PO(0),
};

int get_stack_delta(enum OpCodeType typ) {
  return stack_deltas[typ] - ST_PO(0);
}

// short bad_commands(CyrVM *vm, Stack *stack, enum OpCodeType typ,
//                    union OpCodeData dat) {
//   return handlers[typ](vm, stack, dat);
// }

void cyr_vm_execute(CyrVM *cyr_vm) {
  Stack stack;
  stack_init(&stack);
  OpCode *op_ptr = cyr_vm->codes->items;
  union OpCodeData dat = op_ptr->data;
  enum OpCodeType typ = op_ptr->typ;
#ifndef NO_DEBUG
  int cnts = 0;
  // int bad_cnts = 0;
#endif
  while (1) {
#ifndef NO_DEBUG
    cnts++;
    printf("%s\n", op_code_type(op_ptr->typ));
#endif
    typ = op_ptr->typ;
    dat = op_ptr->data;
    stack.top += op_ptr->stack_delta;
#define EXEC(x) x(cyr_vm, &stack, dat)
    switch (typ) {
    case OP_LOAD_CONST:
      EXEC(load_const);
      break;
    case OP_LOAD_INT:
      EXEC(load_int);
      break;
    case OP_LOAD_ARR:
      EXEC(load_arr);
      break;
    case OP_STORE_INT:
      EXEC(store_int);
      break;
    case OP_STORE_ARR:
      EXEC(store_arr);
      break;
    case OP_SETI:
      EXEC(seti);
      break;
    case OP_INCR:
      EXEC(incr);
      break;
    case OP_INCI:
      EXEC(inci);
      break;
    case OP_CMUL:
      EXEC(cmul);
      break;
    case OP_BINADD:
      EXEC(binadd);
      break;
    case OP_PUT:
      EXEC(put);
      break;
    case OP_JMP:
      op_ptr += EXEC(jmp);
      continue;
    case OP_CJMP:
      op_ptr += EXEC(cjmp);
      continue;
    case OP_HALT:
#ifndef NO_DEBUG
      // printf("\nDispatched bad commands of %d(%d%%)\n", bad_cnts,
      //        bad_cnts * 100 / cnts);
      // printf("Dispatched good commands of %d(%d%%)\n", cnts - bad_cnts,
      //        100 - (bad_cnts * 100 / cnts));
      printf("Dispatched commands of %d(100%%)\n", cnts);
#endif
      return;
    default:
      __builtin_unreachable();
    }
    op_ptr++;
  }
}

#endif