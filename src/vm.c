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

int *arr_ref(VarDecl *decl, int idx) {
  return &decl->data.a.arr[idx - decl->start];
}

int *int_ref(VarDecl *decl) { return &decl->data.i.val; }

typedef short (*vm_handler)(Stack *stack, union OpCodeData dat);

short load_const(Stack *stack, union OpCodeData dat) {
  stack->top[-1] = dat.constant;
  return 1;
}

short load_int(Stack *stack, union OpCodeData dat) {
  stack->top[-1] = *int_ref(dat.ptr);
  return 1;
}

short load_arr(Stack *stack, union OpCodeData dat) {
  int idx = stack->top[-1];
  stack->top[-1] = *arr_ref(dat.ptr, idx);
  return 1;
}

short store_int(Stack *stack, union OpCodeData dat) {
  *int_ref(dat.ptr) = stack->top[0];
  return 1;
}

short store_arr(Stack *stack, union OpCodeData dat) {
  int idx = stack->top[1];
  *arr_ref(dat.ptr, idx) = stack->top[0];
  return 1;
}

short seti(Stack *stack, union OpCodeData dat) {
  *int_ref(dat.var_const.ptr) = dat.var_const.constant;
  return 1;
}

short jmp(Stack *stack, union OpCodeData dat) { return dat.offset; }

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

short cjmp(Stack *stack, union OpCodeData dat) {
  int left = stack->top[1];
  int right = stack->top[0];

  return do_cmp(dat.cjmp.cmp_typ, left, right) ? dat.cjmp.offset : 1;
}

// short cmp(Stack *stack, union OpCodeData dat) {
//   int *left = --stack->top;
//   int *right = stack->top - 1;
//   *right = do_cmp(dat.cmp_typ, *left, *right);
//   return 1;
// }

short incr(Stack *stack, union OpCodeData dat) {
  stack->top[-1] += dat.constant;
  return 1;
}

short inci(Stack *stack, union OpCodeData dat) {
  *int_ref(dat.var_const.ptr) += dat.var_const.constant;
  return 1;
}

short binadd(Stack *stack, union OpCodeData dat) {
  stack->top[-1] += stack->top[0];
  return 1;
}

short triadd(Stack *stack, union OpCodeData dat) {
  stack->top[-1] += stack->top[0] + stack->top[1];
  return 1;
}

short quadadd(Stack *stack, union OpCodeData dat) {
  stack->top[-1] += stack->top[0] + stack->top[1] + stack->top[2];
  return 1;
}

short adds(Stack *stack, union OpCodeData dat) {
  stack->top -= dat.term_cnts;
  int *res = stack->top;
  for (int i = 1; i < dat.term_cnts; i++) {
    *res += stack->top[i];
  }
  stack->top++;
  return 1;
}

short cmul(Stack *stack, union OpCodeData dat) {
  stack->top[-1] *= dat.constant;
  return 1;
}

short empty_func(Stack *stack, union OpCodeData dat) { return 1; }

short put(Stack *stack, union OpCodeData dat) {
  printf("%d ", stack->top[0]);
  return 1;
}

static vm_handler handlers[] = {
    [OP_LOAD_CONST] = load_const,
    [OP_LOAD_INT] = load_int,
    [OP_LOAD_ARR] = load_arr,
    [OP_STORE_INT] = store_int,
    [OP_STORE_ARR] = store_arr,
    [OP_SETI] = seti,
    [OP_JMP] = jmp,
    [OP_CJMP] = cjmp,
    [OP_INCR] = incr,
    [OP_INCI] = inci,
    [OP_BINADD] = binadd,
    [OP_TRIADD] = triadd,
    [OP_QUADADD] = quadadd,
    [OP_ADDS] = adds,
    [OP_CMUL] = cmul,
    [OP_PUT] = put,
    [OP_HALT] = empty_func,
};

#define ST_PO(x) (0 + (x))

static int stack_preoff[] = {
    [OP_LOAD_CONST] = ST_PO(1), [OP_LOAD_INT] = ST_PO(1),
    [OP_LOAD_ARR] = ST_PO(0),   [OP_STORE_INT] = ST_PO(-1),
    [OP_STORE_ARR] = ST_PO(-2), [OP_SETI] = ST_PO(0),
    [OP_JMP] = ST_PO(0),        [OP_CJMP] = ST_PO(-2),
    [OP_INCR] = ST_PO(0),       [OP_INCI] = ST_PO(0),
    [OP_BINADD] = ST_PO(-1),    [OP_TRIADD] = ST_PO(-2),
    [OP_QUADADD] = ST_PO(-3),   [OP_ADDS] = ST_PO(0),
    [OP_CMUL] = ST_PO(0),       [OP_PUT] = ST_PO(-1),
    [OP_HALT] = ST_PO(0),
};

int get_sp_preoff(enum OpCodeType typ) { return stack_preoff[typ] - ST_PO(0); }

short bad_commands(Stack *stack, OpCode *op) {
  return handlers[op->typ](stack, op->data);
}

void cyr_vm_execute(CyrVM *cyr_vm) {
  OpCode *op_ptr = cyr_vm->codes->items;
  Stack stack;
  stack_init(&stack);
  union OpCodeData dat = op_ptr->data;
  int off = 0;
  int cnts = 0;
  int bad_cnts = 0;
  while (1) {
    cnts++;
#ifndef NO_DEBUG
    // printf("%s\n", op_code_type(op_ptr->typ));
#endif
    dat = op_ptr->data;
    stack.top += get_sp_preoff(op_ptr->typ);
    switch (op_ptr->typ) {
    case OP_LOAD_CONST:
      load_const(&stack, dat);
      break;
    case OP_LOAD_INT:
      load_int(&stack, dat);
      break;
    case OP_LOAD_ARR:
      load_arr(&stack, dat);
      break;
    case OP_STORE_INT:
      store_int(&stack, dat);
      break;
    case OP_STORE_ARR:
      store_arr(&stack, dat);
      break;
    case OP_SETI:
      seti(&stack, dat);
      break;
    case OP_CMUL:
      cmul(&stack, dat);
      break;
    case OP_BINADD:
      binadd(&stack, dat);
      break;
    case OP_INCR:
      incr(&stack, dat);
      break;
    case OP_INCI:
      inci(&stack, dat);
      break;
    case OP_JMP:
      off = jmp(&stack, dat);
      break;
    case OP_CJMP:
      off = cjmp(&stack, dat);
      break;
    default:
      bad_cnts++;
      bad_commands(&stack, op_ptr);
      break;
    case OP_HALT:
      goto end;
    }
    if (!off) {
      op_ptr++;
    } else {
      op_ptr += off;
      off = 0;
    }
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