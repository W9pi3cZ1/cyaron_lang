#ifdef CODEGEN

#ifndef NO_STD_INC
#include <stddef.h>
#include <stdlib.h>
#endif

#ifndef NO_CUSTOM_INC
#include "codegen.h"
#include "parser.h"
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

size_t gen_load_const(CodeGen *cg, int constant) {
  OpCode *constant_op = da_try_push_back(&cg->codes);
  constant_op->typ = OP_LOAD_CONST;
  constant_op->data.constant = constant;
  return cg->codes.item_cnts - 1;
}

size_t gen_cmul(CodeGen *cg, int constant) {
  OpCode *constant_op = da_try_push_back(&cg->codes);
  constant_op->typ = OP_CMUL;
  constant_op->data.constant = constant;
  return cg->codes.item_cnts - 1;
}

size_t gen_adds(CodeGen *cg, int term_cnts) {
  switch (term_cnts) {
  case 1:
    return cg->codes.item_cnts - 1; // unreachable
  case 2: {
    OpCode *binadd_op = da_try_push_back(&cg->codes);
    binadd_op->typ = OP_BINADD;
    return cg->codes.item_cnts - 1;
  }
  case 3: {
    OpCode *triadd_op = da_try_push_back(&cg->codes);
    triadd_op->typ = OP_TRIADD;
    return cg->codes.item_cnts - 1;
  }
  case 4: {
    OpCode *quadadd_op = da_try_push_back(&cg->codes);
    quadadd_op->typ = OP_QUADADD;
    return cg->codes.item_cnts - 1;
  }
  default:
    break;
  }
  OpCode *adds_op = da_try_push_back(&cg->codes);
  adds_op->typ = OP_ADDS;
  adds_op->data.term_cnts = term_cnts;
  return cg->codes.item_cnts - 1;
}

size_t gen_put(CodeGen *cg) {
  OpCode *put_op = da_try_push_back(&cg->codes);
  put_op->typ = OP_PUT;
  return cg->codes.item_cnts - 1;
}

size_t gen_jmp(CodeGen *cg, short offset) {
  OpCode *jmp_op = da_try_push_back(&cg->codes);
  jmp_op->typ = OP_JMP;
  jmp_op->data.offset = offset;
  return cg->codes.item_cnts - 1;
}

// size_t gen_jz(CodeGen *cg, short offset) {
//   OpCode *jz_op = da_try_push_back(&cg->codes);
//   jz_op->typ = OP_JZ;
//   jz_op->data.offset = offset;
//   return cg->codes.item_cnts - 1;
// }

// size_t gen_jnz(CodeGen *cg, short offset) {
//   OpCode *jnz_op = da_try_push_back(&cg->codes);
//   jnz_op->typ = OP_JNZ;
//   jnz_op->data.offset = offset;
//   return cg->codes.item_cnts - 1;
// }

size_t gen_incr(CodeGen *cg, int constant) {
  OpCode *incr_op = da_try_push_back(&cg->codes);
  incr_op->typ = OP_INCR;
  incr_op->data.constant = constant;
  return cg->codes.item_cnts - 1;
}

static inline enum CmpType reverse_cmp_typ(enum CmpType cmp_typ) {
  switch (cmp_typ) {
  case CMP_LT:
    return CMP_GE;
  case CMP_GT:
    return CMP_LE;
  case CMP_EQ:
    return CMP_NEQ;
  case CMP_LE:
    return CMP_GT;
  case CMP_GE:
    return CMP_LT;
  case CMP_NEQ:
    return CMP_EQ;
  }
}

size_t gen_jcmp(CodeGen *cg, enum CmpType cmp_typ, short offset) {
  OpCode *jcmp_op = da_try_push_back(&cg->codes);
  switch (cmp_typ) {
  case CMP_LT:
    jcmp_op->typ = OP_JLT;
    break;
  case CMP_GT:
    jcmp_op->typ = OP_JGT;
    break;
  case CMP_EQ:
    jcmp_op->typ = OP_JEQ;
    break;
  case CMP_LE:
    jcmp_op->typ = OP_JLE;
    break;
  case CMP_GE:
    jcmp_op->typ = OP_JGE;
    break;
  case CMP_NEQ:
    jcmp_op->typ = OP_JNEQ;
    break;
  }
  // cmp_op->typ = OP_CMP;
  jcmp_op->data.offset = offset;
  return cg->codes.item_cnts - 1;
}

size_t gen_halt(CodeGen *cg) {
  OpCode *halt_op = da_try_push_back(&cg->codes);
  halt_op->typ = OP_HALT;
  return cg->codes.item_cnts - 1;
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
  // 保证项的系数从高到低排
  OperandTerm *op_term = expr->op_terms.items;

  int term_count = expr->op_terms.item_cnts;
  if (!term_count) {
    gen_load_const(cg, expr->constant);
    return; // 空表达式
  }

  // 首先处理常数部分
  if (expr->constant != 0) {
    // 特判可优化成INCR的情况
    if (term_count == 1 && op_term[0].coefficient != 0) {
      gen_load_operand(cg, &op_term[0].operand);
      if (op_term[0].coefficient != 1) {
        gen_cmul(cg, op_term[0].coefficient);
      }
      gen_incr(cg, expr->constant);
      return;
    } else {
      gen_load_const(cg, expr->constant);
    }
  }

  int has_constant_on_stack = (expr->constant != 0) ? 1 : 0;
  int stack_items = has_constant_on_stack;

  // 先跳过所有系数为0的项，找到第一个非零系数的项
  int start = 0;
  while (start < term_count && op_term[start].coefficient == 0) {
    start++;
  }

  if (start == term_count) {
    // 所有项的系数都是0，只有常数部分（如果存在）
    return;
  }

  // 按系数分组处理
  int current_coeff = op_term[start].coefficient;
  int group_start = start;
  int group_size = 0;

  for (int i = start; i < term_count; i++) {
    // 跳过系数为0的项
    if (op_term[i].coefficient == 0) {
      continue;
    }

    // 检查系数是否变化
    if (op_term[i].coefficient != current_coeff) {
      // 处理当前分组
      if (group_size > 0) {
        // 生成当前分组的加法（如果有多项）
        if (group_size > 1) {
          gen_adds(cg, group_size);
          stack_items -= (group_size - 1); // 合并后减少栈项数
        }

        // 处理系数乘法（如果系数不是1或-1）
        if (current_coeff != 1) {
          gen_cmul(cg, current_coeff);
          // 乘法消耗2个操作数，产生1个结果，栈项数不变
        }
        // 系数为1时不需要乘法

        // 重置分组
        group_start = i;
        group_size = 0;
        current_coeff = op_term[i].coefficient;
      }
    }

    // 加载当前操作数
    gen_load_operand(cg, &op_term[i].operand);
    stack_items++;
    group_size++;
  }

  // 处理最后一组
  if (group_size > 0) {
    // 生成当前分组的加法（如果有多项）
    if (group_size > 1) {
      gen_adds(cg, group_size);
      stack_items -= (group_size - 1);
    }

    // 处理系数乘法
    if (current_coeff != 1 && current_coeff != -1) {
      gen_cmul(cg, current_coeff);
    } else if (current_coeff == -1) {
      gen_cmul(cg, -1);
    }
  }

  // 最后，如果栈上有多项（包括常数和各项结果），进行加法合并
  if (stack_items > 1) {
    gen_adds(cg, stack_items);
  }
}

// void gen_expr(CodeGen *cg, Expr *expr) {
//   // 保证项的系数从高到低排
//   OperandTerm *op_term = expr->op_terms.items;

//   int term_count = expr->op_terms.item_cnts;
//   if (!term_count) {
//     gen_load_const(cg, expr->constant);
//     return; // 空表达式
//   }

//   if (expr->constant != 0) {
//     gen_load_const(cg, expr->constant);
//   }

//   // 分组处理相同系数的项
//   int current_coeff = op_term->coefficient;
//   int group_start = 0;
//   int has_constant_on_stack = (expr->constant != 0) ? 1 : 0;
//   int stack_items = has_constant_on_stack;

//   for (int i = 0; i <= term_count; i++) {
//     printf("%d\n", op_term[i].coefficient);
//     // 检测系数变化或到达末尾
//     int coeff_changed = (op_term[i].coefficient != current_coeff);

//     if (i < term_count && current_coeff != 0) {
//       gen_load_operand(cg, &op_term[i].operand);
//       stack_items++;
//     }

//     if (!coeff_changed || i < term_count)
//       continue;

//     int group_size = i - group_start;

//     if (current_coeff == 1) {
//       if (stack_items > 1)
//         gen_adds(cg, stack_items);
//       stack_items = 1;
//     } else if (current_coeff != 0) {
//       // 处理组内加法（如果组内有多个项）
//       if (group_size > 1) {
//         gen_adds(cg, group_size);
//         stack_items -= (group_size - 1); // 合并为1项
//       }
//       // 处理系数乘法（如果系数不是1）
//       gen_cmul(cg, current_coeff);
//     }

//     // 准备下一个分组
//     if (i < term_count) {
//       current_coeff = op_term[i].coefficient;
//       group_start = i;
//     }
//   }
//   if (stack_items > 1)
//     gen_adds(cg, stack_items);
// }

size_t gen_cond(CodeGen *cg, Cond *cond, short offset) {
  gen_expr(cg, &cond->right);
  gen_expr(cg, &cond->left);
  return gen_jcmp(cg, cond->typ, offset);
}

static OpCode *get_opcode(CodeGen *cg, size_t idx) {
  return (OpCode *)da_get(&cg->codes, idx);
}

void gen_stmts(CodeGen *cg, DynArr *stmts) {
  Stmt *stmt_ptr = stmts->items;
  for (int i = 0; i < stmts->item_cnts; i++, stmt_ptr++) {
    switch (stmt_ptr->typ) {
    case STMT_IHU_BLK: {
      IhuStmt *ihu_stmt = &stmt_ptr->inner.ihu;
      ihu_stmt->cond.typ = reverse_cmp_typ(ihu_stmt->cond.typ);
      size_t try_skip = gen_cond(cg, &ihu_stmt->cond, 0);
      int offset = try_skip + 1;
      gen_stmts(cg, &ihu_stmt->stmts);
      offset = cg->codes.item_cnts - offset + 1;
      get_opcode(cg, try_skip)->data.offset = offset; // avoid use after free
    } break;
    case STMT_WHILE_BLK: {
      WhileStmt *while_stmt = &stmt_ptr->inner.while_stmt;
      size_t jmp_cond = gen_jmp(cg, 0);
      int stmts_begin = jmp_cond + 1;
      gen_stmts(cg, &while_stmt->stmts);
      int stmts_end = cg->codes.item_cnts;
      size_t try_continue = gen_cond(cg, &while_stmt->cond, 0);
      int cond_end = try_continue + 1;
      get_opcode(cg, try_continue)->data.offset = stmts_begin - cond_end + 1;
      get_opcode(cg, jmp_cond)->data.offset = stmts_end - stmts_begin + 1;
    } break;
    case STMT_HOR_BLK: {
      HorStmt *hor_stmt = &stmt_ptr->inner.hor;
      gen_expr(cg, &hor_stmt->start);
      gen_store_operand(cg, &hor_stmt->var);
      size_t try_skip = gen_jmp(cg, 0);
      int stmts_begin = try_skip + 1;
      gen_stmts(cg, &hor_stmt->stmts);
      gen_load_operand(cg, &hor_stmt->var);
      gen_incr(cg, 1);
      gen_store_operand(cg, &hor_stmt->var);
      int stmts_end = cg->codes.item_cnts;
      gen_expr(cg, &hor_stmt->end);
      gen_load_operand(cg, &hor_stmt->var);
      size_t try_continue = gen_jcmp(cg, CMP_LE, 0);
      int cond_end = try_continue + 1;
      get_opcode(cg, try_continue)->data.offset = stmts_begin - cond_end + 1;
      get_opcode(cg, try_skip)->data.offset = stmts_end - stmts_begin + 1;
    } break;
    case STMT_YOSORO_CMD: {
      YosoroStmt *yosoro_stmt = &stmt_ptr->inner.yosoro;
      gen_expr(cg, &yosoro_stmt->expr);
      gen_put(cg);
      break;
    }
    case STMT_SET_CMD: {
      SetStmt *set_stmt = &stmt_ptr->inner.set;
      Expr *expr = &set_stmt->expr;
      OperandTerm *op_term_ptr = expr->op_terms.items;
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
    case OP_INCR:
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
    case OP_JLT:
    case OP_JGT:
    case OP_JEQ:
    case OP_JLE:
    case OP_JGE:
    case OP_JNEQ:
      printf("offset: %hd", code_ptr->data.offset);
      break;
    // case OP_CMP:
    //   printf("cmp_typ: %s",
    //          debug_token_type(code_ptr->data.cmp_typ + TOK_CMP_LT - 1));
    //   break;
    case OP_ADDS:
      printf("term_cnts: %d", code_ptr->data.term_cnts);
      break;
    case OP_BINADD:
    case OP_TRIADD:
    case OP_QUADADD:
    case OP_PUT:
    case OP_HALT:
      break;
    }
    puts(")");
  }
}
#endif

#endif