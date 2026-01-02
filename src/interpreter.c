#ifndef CODEGEN

#ifndef NO_CUSTOM_INC
#include "interpreter.h"
#include "parser.h"
#include "utils.h"
#endif

#ifndef NO_STD_INC
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

// void var_decls_init_data(DynArr *var_decls) {
//   VarDecl *var_decl = var_decls->items;
//   for (int i = 0; i < var_decls->item_cnts; i++, var_decl++) {
//     switch (var_decl->typ) {
//     case VAR_INT: {
//       VarIntData *int_data = &var_decl->data.i;
//       int_data->val = 0;
//     } break;
//     case VAR_ARR: {
//       VarArrData *arr_data = &var_decl->data.a;
//       arr_data->arr =
//           malloc((var_decl->end - var_decl->start + 1) * sizeof(int));
//     } break;
//     }
//   }
// }

// void var_decls_free_data(DynArr *var_decls) {
//   VarDecl *var_decl = var_decls->items;
//   for (int i = 0; i < var_decls->item_cnts; i++, var_decl++) {
//     switch (var_decl->typ) {
//     case VAR_INT:
//       break;
//     case VAR_ARR: {
//       VarArrData *arr_data = &var_decl->data.a;
//       free(arr_data->arr);
//     } break;
//     }
//   }
// }

void interpreter_init(Interpreter *interpreter, DynArr *stmts,
                      DynArr *var_decls) {
  memset(interpreter, 0, sizeof(Interpreter));
  interpreter->stmts = stmts;
  // interpreter->exec_ptr = 0;
  // var_decls_init_data(var_decls);
  interpreter->var_decls = var_decls;
}

Interpreter *interpreter_create(DynArr *stmts, DynArr *var_decls) {
  Interpreter *interpreter = malloc(sizeof(Interpreter));
  interpreter_init(interpreter, stmts, var_decls);
  return interpreter;
}

void interpreter_free(Interpreter *interpreter) {
  // var_decls_free_data(interpreter->var_decls);
}

int eval_expr(Interpreter *interpreter, Expr *expr);

__attribute__((noinline)) // Keep it noinline
static int compute_array_index(Interpreter *interpreter, Expr *idx_expr,
                               int start) {
  int idx_res = eval_expr(interpreter, idx_expr);
  // if (idx_res > decl->end || idx_res < decl->start) {
  //   err_log("Index(%d) of Array(#%zu) is Out of Bounds (in %s)\n", idx_res,
  //           operand->decl_idx, __FUNCTION__);
  // }
  return idx_res - start;
}

int *operand_get_ref(Interpreter *interpreter, Operand *operand) {
  VarDecl *decl =
      (VarDecl *)(interpreter->var_decls->items) + operand->decl_idx;
  if (operand->typ == OPERAND_INT_VAR) {
    return &decl->data.i.val;
  }
  // else OPERAND_ARR_ELEM
  int idx = compute_array_index(interpreter, &operand->idx_expr, decl->start);
  return &decl->data.a.arr[idx];
}

void operand_write(Interpreter *interpreter, Operand *operand, int data) {
#ifndef NO_DEBUG
  interpreter->stats.operand_write++;
#endif
  *operand_get_ref(interpreter, operand) = data;
}

int operand_read(Interpreter *interpreter, Operand *operand) {
#ifndef NO_DEBUG
  interpreter->stats.operand_read++;
#endif
  return *operand_get_ref(interpreter, operand);
}

int eval_expr(Interpreter *interpreter, Expr *expr) {
#ifndef NO_DEBUG
  interpreter->stats.expression_eval++;
#endif
  DynArr *op_terms = &expr->op_terms;
  int res = expr->constant;
  OperandTerm *op_term = op_terms->items;
  for (int i = 0; i < op_terms->item_cnts; ++i, ++op_term) {
    int val = operand_read(interpreter, &op_term->operand);
    res += val * op_term->coefficient;
  }
  return res;
}

void execute_stmts(Interpreter *interpreter, DynArr *stmts);

char execute_cond(Interpreter *interpreter, Cond *cond) {
  int left = eval_expr(interpreter, &cond->left);
  int right = eval_expr(interpreter, &cond->right);
  return do_cmp(cond->typ, left, right);
}

typedef void (*StmtHandler)(Interpreter *interpreter, Stmt *stmt);

static void execute_yosoro(Interpreter *interpreter, Stmt *stmt) {
  YosoroStmt *yosoro = &stmt->inner.yosoro;
  int res = eval_expr(interpreter, &yosoro->expr);
  printf("%d ", res);
}

static void execute_set(Interpreter *interpreter, Stmt *stmt) {
  SetStmt *set = &stmt->inner.set;
  int res = eval_expr(interpreter, &set->expr);
  operand_write(interpreter, &set->operand, res);
}

static void execute_ihu(Interpreter *interpreter, Stmt *stmt) {
  IhuStmt *ihu = &stmt->inner.ihu;
  Cond *cond = &ihu->cond;
  DynArr *stmts = &ihu->stmts;
  if (!execute_cond(interpreter, cond))
    return;
  execute_stmts(interpreter, stmts);
}

static void execute_while(Interpreter *interpreter, Stmt *stmt) {
  WhileStmt *while_stmt = &stmt->inner.while_stmt;
  Cond *cond = &while_stmt->cond;
  DynArr *stmts = &while_stmt->stmts;
  while (execute_cond(interpreter, cond)) {
    execute_stmts(interpreter, stmts);
  }
}

static void execute_hor(Interpreter *interpreter, Stmt *stmt) {
  HorStmt *hor = &stmt->inner.hor;
  int start = eval_expr(interpreter, &hor->start);
  int end = eval_expr(interpreter, &hor->end);
  Operand *var = &hor->var;
  DynArr *stmts = &hor->stmts;
  for (int i = start; i <= end; i++) {
    operand_write(interpreter, var, i);
    execute_stmts(interpreter, stmts);
  }
}

void execute_stmt(Interpreter *interpreter, Stmt *stmt) {
  static StmtHandler handlers[] = {
      execute_ihu, execute_while, execute_hor, execute_yosoro, execute_set,
  };
  StmtHandler handler = handlers[stmt->typ];
  handler(interpreter, stmt);
}

inline void execute_stmts(Interpreter *interpreter, DynArr *stmts) {
  Stmt *stmt = stmts->items;
  for (int i = 0; i < stmts->item_cnts; i++, stmt++) {
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

#endif