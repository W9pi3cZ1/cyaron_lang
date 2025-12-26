#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef NO_CUSTOM_INC
#include "codegen.h"
// #include "parser.h"
#include "utils.h"
#endif

void cg_init(CodeGen *cg, DynArr *stmts, DynArr *var_decls) {
  da_init(&cg->codes, sizeof(OpCode), 64);
  cg->stmts = stmts;
  cg->var_decls = var_decls;
}

CodeGen *cg_create(DynArr *stmts, DynArr *var_decls) {
  CodeGen *cg = malloc(sizeof(CodeGen));
  cg_init(cg, stmts, var_decls);
  return cg;
}

void cg_free(CodeGen *cg) { da_free(&cg->codes); }

OpCode *gen_load_const(CodeGen *cg, int constant) {
  OpCode *constant_op = da_try_push_back(&cg->codes);
  constant_op->typ = OP_LOAD_CONST;
  constant_op->data.constant = constant;
  return constant_op;
}

OpCode *gen_cmul(CodeGen *cg, int constant) {
  OpCode *constant_op = da_try_push_back(&cg->codes);
  constant_op->typ = OP_CMUL;
  constant_op->data.constant = constant;
  return constant_op;
}

OpCode *gen_adds(CodeGen *cg, int term_cnts) {
  OpCode *adds_op = da_try_push_back(&cg->codes);
  adds_op->typ = OP_ADDS;
  adds_op->data.term_cnts = term_cnts;
  return adds_op;
}

OpCode *gen_put(CodeGen *cg) {
  OpCode *put_op = da_try_push_back(&cg->codes);
  put_op->typ = OP_PUT;
  return put_op;
}

OpCode *gen_jmp(CodeGen *cg, short offset) {
  OpCode *jmp_op = da_try_push_back(&cg->codes);
  jmp_op->typ = OP_JMP;
  jmp_op->data.offset = offset;
  return jmp_op;
}

OpCode *gen_jz(CodeGen *cg, short offset) {
  OpCode *jz_op = da_try_push_back(&cg->codes);
  jz_op->typ = OP_JZ;
  jz_op->data.offset = offset;
  return jz_op;
}

OpCode *gen_jnz(CodeGen *cg, short offset) {
  OpCode *jnz_op = da_try_push_back(&cg->codes);
  jnz_op->typ = OP_JNZ;
  jnz_op->data.offset = offset;
  return jnz_op;
}

OpCode *gen_incr(CodeGen *cg) {
  OpCode *incr_op = da_try_push_back(&cg->codes);
  incr_op->typ = OP_INCR;
  return incr_op;
}

OpCode *gen_cmp(CodeGen *cg, enum CmpType cmp_typ) {
  OpCode *cmp_op = da_try_push_back(&cg->codes);
  cmp_op->typ = OP_CMP;
  cmp_op->data.cmp_typ = cmp_typ;
  return cmp_op;
}

OpCode *gen_halt(CodeGen *cg) {
  OpCode *halt_op = da_try_push_back(&cg->codes);
  halt_op->typ = OP_HALT;
  return halt_op;
}

void gen_expr(CodeGen *cg, Expr *expr);

void gen_load_operand(CodeGen *cg, Operand *operand) {
  switch (operand->typ) {
  case OPERAND_INT_VAR: {
    OpCode *int_op = da_try_push_back(&cg->codes);
    int_op->data.ptr = (VarDecl *)(cg->var_decls->items) + operand->decl_idx;
    int_op->typ = OP_LOAD_INT;
  } break;
  case OPERAND_ARR_ELEM: {
    // Load Idx
    gen_expr(cg, &operand->idx_expr);
    OpCode *arr_op = da_try_push_back(&cg->codes);
    arr_op->data.ptr = (VarDecl *)(cg->var_decls->items) + operand->decl_idx;
    arr_op->typ = OP_LOAD_ARR;
  } break;
  }
}

void gen_store_operand(CodeGen *cg, Operand *operand) {
  switch (operand->typ) {
  case OPERAND_INT_VAR: {
    OpCode *int_op = da_try_push_back(&cg->codes);
    int_op->data.ptr = (VarDecl *)(cg->var_decls->items) + operand->decl_idx;
    int_op->typ = OP_STORE_INT;
  } break;
  case OPERAND_ARR_ELEM: {
    // Load Idx
    gen_expr(cg, &operand->idx_expr);
    OpCode *arr_op = da_try_push_back(&cg->codes);
    arr_op->data.ptr = (VarDecl *)(cg->var_decls->items) + operand->decl_idx;
    arr_op->typ = OP_STORE_ARR;
  } break;
  }
}

void gen_expr(CodeGen *cg, Expr *expr) {
  // 保证系数都不为0，而且项的系数从高到低排
  OperandTerm *op_term = expr->op_terms.items;

  int term_count = expr->op_terms.item_cnts;
  if (!term_count) {
    gen_load_const(cg, expr->constant);
    return; // 所有项系数都为0
  }

  if (expr->constant != 0) {
    gen_load_const(cg, expr->constant);
  }

  // 分组处理相同系数的项
  int current_coeff = op_term->coefficient;
  int group_start = 0;
  int has_constant_on_stack = (expr->constant != 0) ? 1 : 0;
  int stack_items = has_constant_on_stack;

  for (int i = 0; i <= term_count; i++) {
    // 检测系数变化或到达末尾
    int coeff_changed = (op_term[i].coefficient != current_coeff);

    if (i < term_count) {
      gen_load_operand(cg, &op_term[i].operand);
      stack_items++;
    }

    if (!(coeff_changed || i < term_count))
      continue;

    int group_size = i - group_start;

    if (current_coeff == 1) {
      if (stack_items > 1)
        gen_adds(cg, stack_items);
      stack_items = 1;
    } else {
      // 处理组内加法（如果组内有多个项）
      if (group_size > 1) {
        gen_adds(cg, group_size);
        stack_items -= (group_size - 1); // 合并为1项
      }
      // 处理系数乘法（如果系数不是1）
      gen_cmul(cg, current_coeff);
    }

    // 准备下一个分组
    if (i < term_count) {
      current_coeff = op_term[i].coefficient;
      group_start = i;
    }
  }
  if (stack_items > 1)
    gen_adds(cg, stack_items);
}

void gen_cond(CodeGen *cg, Cond *cond) {
  gen_expr(cg, &cond->right);
  gen_expr(cg, &cond->left);
  gen_cmp(cg, cond->typ);
}

void gen_stmts(CodeGen *cg, DynArr *stmts) {
  Stmt *stmt_ptr = stmts->items;
  for (int i = 0; i < stmts->item_cnts; i++, stmt_ptr++) {
    switch (stmt_ptr->typ) {
    case STMT_IHU_BLK: {
      IhuStmt *ihu_stmt = &stmt_ptr->inner.ihu;
      gen_cond(cg, &ihu_stmt->cond);
      int offset = cg->codes.item_cnts;
      OpCode *try_skip = gen_jz(cg, 0);
      gen_stmts(cg, &ihu_stmt->stmts);
      offset = cg->codes.item_cnts - offset;
      try_skip->data.offset = offset;
    } break;
    case STMT_WHILE_BLK: {
      WhileStmt *while_stmt = &stmt_ptr->inner.while_stmt;
      OpCode *try_skip = gen_jmp(cg, 0);
      int stmts_begin = cg->codes.item_cnts;
      gen_stmts(cg, &while_stmt->stmts);
      int stmts_end = cg->codes.item_cnts;
      gen_cond(cg, &while_stmt->cond);
      int cond_end = cg->codes.item_cnts;
      OpCode *try_continue = gen_jnz(cg, stmts_begin - cond_end);
      try_skip->data.offset = stmts_end - stmts_begin + 1;
    } break;
    case STMT_HOR_BLK: {
      HorStmt *hor_stmt = &stmt_ptr->inner.hor;
      gen_expr(cg, &hor_stmt->start);
      gen_store_operand(cg, &hor_stmt->var);
      OpCode *try_skip = gen_jmp(cg, 0);
      int stmts_begin = cg->codes.item_cnts;
      gen_stmts(cg, &hor_stmt->stmts);
      gen_load_operand(cg, &hor_stmt->var);
      gen_incr(cg);
      gen_store_operand(cg, &hor_stmt->var);
      int stmts_end = cg->codes.item_cnts;
      gen_expr(cg, &hor_stmt->end);
      gen_load_operand(cg, &hor_stmt->var);
      gen_cmp(cg, CMP_LE);
      int cond_end = cg->codes.item_cnts;
      OpCode *try_continue = gen_jnz(cg, stmts_begin - cond_end);
      try_skip->data.offset = stmts_end - stmts_begin + 1;
    } break;
    case STMT_YOSORO_CMD: {
      YosoroStmt *yosoro_stmt = &stmt_ptr->inner.yosoro;
      gen_expr(cg, &yosoro_stmt->expr);
      gen_put(cg);
      break;
    }
    case STMT_SET_CMD: {
      SetStmt *set_stmt = &stmt_ptr->inner.set;
      gen_expr(cg, &set_stmt->expr);
      gen_store_operand(cg, &set_stmt->operand);
      break;
    }
    }
  }
}

void cg_gen(CodeGen *cg) {
  gen_stmts(cg, cg->stmts);
  gen_halt(cg);
}

#ifndef NO_DEBUG
const char *op_code_type(enum OpCodeType op_typ) {
  switch (op_typ) {
#undef OPCODE
#define OPCODE(x)                                                              \
  case x:                                                                      \
    return #x;
#include "opcodes.h"
#undef OPCODE
  default:
    return "<Invalid>";
  }
}

void cg_debug(CodeGen *cg) {
  OpCode *code_ptr = cg->codes.items;
  for (int i = 0; i < cg->codes.item_cnts; i++, code_ptr++) {
    printf("[%d] = %s(", i, op_code_type(code_ptr->typ));
    switch (code_ptr->typ) {
    case OP_LOAD_CONST:
    case OP_CMUL:
      printf("const: %d", code_ptr->data.constant);
      break;
    case OP_LOAD_INT:
    case OP_LOAD_ARR:
    case OP_STORE_INT:
    case OP_STORE_ARR: {
      VarDecl *decl_ptr = code_ptr->data.ptr;
      printf("ptr: %p(#%hd)", decl_ptr, decl_ptr->decl_idx);
    } break;
    case OP_JMP:
    case OP_JZ:
    case OP_JNZ:
      printf("offset: %hd", code_ptr->data.offset);
      break;
    case OP_CMP:
      printf("cmp_typ: %s",
             debug_token_type(code_ptr->data.cmp_typ + TOK_CMP_LT - 1));
      break;
    case OP_ADDS:
      printf("term_cnts: %d", code_ptr->data.term_cnts);
      break;
    case OP_INCR:
    case OP_PUT:
    case OP_HALT:
      break;
    }
    puts(")");
  }
}
#endif