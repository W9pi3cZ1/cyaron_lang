#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"

#ifndef NO_STD_INC
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#endif

void var_decls_init_data(DynArr *var_decls) {
  for (int i = 0; i < var_decls->item_cnts; i++) {
    VarDecl *var_decl = da_get(var_decls, i);
    switch (var_decl->typ) {
    case VAR_INT: {
      VarIntData *int_data = &var_decl->data.i;
      int_data->val = 0;
    } break;
    case VAR_ARR: {
      VarArrData *arr_data = &var_decl->data.a;
      arr_data->arr =
          malloc((var_decl->end - var_decl->start + 1) * sizeof(int));
    } break;
    }
  }
}

void var_decls_free_data(DynArr *var_decls) {
  for (int i = 0; i < var_decls->item_cnts; i++) {
    VarDecl *var_decl = da_get(var_decls, i);
    switch (var_decl->typ) {
    case VAR_INT:
      break;
    case VAR_ARR: {
      VarArrData *arr_data = &var_decl->data.a;
      free(arr_data->arr);
    } break;
    }
  }
}

Interpreter *interpreter_create(DynArr *stmts, DynArr *var_decls) {
  Interpreter *interpreter = calloc(1, sizeof(Interpreter));
  interpreter->stmts = stmts;
  // interpreter->exec_ptr = 0;
  var_decls_init_data(var_decls);
  interpreter->var_decls = var_decls;
  return interpreter;
}

void interpreter_free(Interpreter *interpreter) {
  var_decls_free_data(interpreter->var_decls);
  free(interpreter);
}

int eval_expr(Interpreter *interpreter, Expr *expr);

static int *operand_get_ref(Interpreter *interpreter, Operand *operand) {
  size_t decl_idx = operand->decl_idx;
  VarDecl *decl = da_get(interpreter->var_decls, decl_idx);
  switch (operand->typ) {
  case OPERAND_INT_VAR:
    return &decl->data.i.val;
  case OPERAND_ARR_ELEM: {
    int idx_res = eval_expr(interpreter, &operand->idx_expr);
    // if (idx_res > decl->end || idx_res < decl->start) {
    //   err_log("Index(%d) of Array(#%zu) is Out of Bounds (in %s)\n", idx_res,
    //           operand->decl_idx, __FUNCTION__);
    // }
    return &decl->data.a.arr[idx_res - decl->start];
  }
  default:
    // err_log("Operand(#%zu) isn't supported (in %s)\n", operand->decl_idx,
    // __FUNCTION__);
    return NULL;
  }
}

inline void operand_write(Interpreter *interpreter, Operand *operand,
                          int data) {
#ifndef NO_DEBUG
  interpreter->stats.operand_write++;
#endif
  *operand_get_ref(interpreter, operand) = data;
}

inline int operand_read(Interpreter *interpreter, Operand *operand) {
#ifndef NO_DEBUG
  interpreter->stats.operand_read++;
#endif
  return *operand_get_ref(interpreter, operand);
}

int eval_expr(Interpreter *interpreter, Expr *expr) {
#ifndef NO_DEBUG
  interpreter->stats.expression_eval++;
#endif
  DynArr *terms = &expr->terms;
  int res = 0;
  for (int i = 0; i < terms->item_cnts; i++) {
    ExprTerm *term = da_get(terms, i);
    switch (term->typ) {
    case EXPR_TERM_OPERAND:
      res += term->coefficient * operand_read(interpreter, &term->operand);
      break;
    case EXPR_TERM_CONST:
      res += term->coefficient * term->constant;
      break;
    }
  }
  return res;
}

void execute_stmts(Interpreter *interpreter, DynArr *stmts);

char assign_cond_old(enum TokType cond_typ, int left, int right) {
  switch (cond_typ) {
  case TOK_CMP_LT:
    return left < right;
  case TOK_CMP_GT:
    return left > right;
  case TOK_CMP_LE:
    return left <= right;
  case TOK_CMP_GE:
    return left >= right;
  case TOK_CMP_EQ:
    return left == right;
  case TOK_CMP_NEQ:
    return left != right;
  default:
    return 0;
  }
}

char assign_cond(enum TokType cond_typ, int left, int right) {
  cond_typ -= TOK_CMP_LT;
  unsigned int lt = (unsigned int)(left < right);
  unsigned int eq = (unsigned int)(left == right);
  unsigned int gt = (unsigned int)(left > right);

  static const char result_table[6] = {
      0b001, // TOK_CMP_LT: lt
      0b100, // TOK_CMP_GT: gt
      0b011, // TOK_CMP_LE: lt || eq
      0b110, // TOK_CMP_GE: gt || eq
      0b010, // TOK_CMP_EQ: eq
      0b101  // TOK_CMP_NEQ: lt || gt
  };

  if (cond_typ >= 0 && cond_typ < 6) {
    unsigned int mask = result_table[cond_typ];
    return (lt & mask) | (eq & (mask >> 1)) | (gt & (mask >> 2));
  }
  return 0;
}

char execute_cond(Interpreter *interpreter, Cond *cond) {
  int left = eval_expr(interpreter, &cond->left);
  int right = eval_expr(interpreter, &cond->right);
  return assign_cond(cond->typ, left, right);
}

void execute_stmt(Interpreter *interpreter, Stmt *stmt) {
  switch (stmt->typ) {
  case STMT_IHU_BLK: {
    IhuStmt *ihu = &stmt->inner.ihu;
    if (execute_cond(interpreter, &ihu->cond)) {
      execute_stmts(interpreter, &ihu->stmts);
    }
  } break;
  case STMT_WHILE_BLK: {
    WhileStmt *while_stmt = &stmt->inner.while_stmt;
    while (execute_cond(interpreter, &while_stmt->cond)) {
      execute_stmts(interpreter, &while_stmt->stmts);
    }
  } break;
  case STMT_HOR_BLK: {
    HorStmt *hor = &stmt->inner.hor;
    int start = eval_expr(interpreter, &hor->start);
    int end = eval_expr(interpreter, &hor->end);
    for (int i = start; i <= end; i++) {
      operand_write(interpreter, &hor->var, i);
      execute_stmts(interpreter, &hor->stmts);
    }
  } break;
  case STMT_YOSORO_CMD: {
    YosoroStmt *yosoro = &stmt->inner.yosoro;
    int res = eval_expr(interpreter, &yosoro->expr);
    printf("%d ", res);
  } break;
  case STMT_SET_CMD: {
    SetStmt *set = &stmt->inner.set;
    int res = eval_expr(interpreter, &set->expr);
    operand_write(interpreter, &set->operand, res);
  } break;
  }
}

void execute_stmts(Interpreter *interpreter, DynArr *stmts) {
  Stmt *stmt;
  for (int i = 0; i < stmts->item_cnts; i++) {
    stmt = da_get(stmts, i);
    execute_stmt(interpreter, stmt);
  }
}

void interpreter_execute(Interpreter *interpreter) {
  DynArr *stmts = interpreter->stmts;
  execute_stmts(interpreter, stmts);
}

#ifndef NO_DEBUG
void interpreter_stats(Interpreter *interpreter) {
  struct InterpreterStats stats = interpreter->stats;
  logger("Interpreter Stats:\n"
         "Operand\n"
         "  Read : %zu\n"
         "  Write: %zu\n"
         "Expression\n"
         "  Eval : %zu\n",
         stats.operand_read, stats.operand_write, stats.expression_eval);
}
#endif