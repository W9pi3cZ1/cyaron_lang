#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"

#ifndef NO_STD_INC
#include <stddef.h>
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
  Interpreter *interpreter = malloc(sizeof(Interpreter));
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

inline void write_operand(Interpreter *interpreter, Operand *operand,
                          int data) {
  size_t decl_idx = operand->decl_idx;
  VarDecl *decl = da_get(interpreter->var_decls, decl_idx);
  switch (operand->typ) {
  case OPERAND_INT_VAR:
    decl->data.i.val = data;
    return;
  case OPERAND_ARR_ELEM:
    decl->data.a.arr[eval_expr(interpreter, &operand->idx_expr) - decl->start] =
        data;
    return;
  }
}

inline int read_operand(Interpreter *interpreter, Operand *operand) {
  size_t decl_idx = operand->decl_idx;
  VarDecl *decl = da_get(interpreter->var_decls, decl_idx);
  int data = 0;
  switch (operand->typ) {
  case OPERAND_INT_VAR:
    data = decl->data.i.val;
    break;
  case OPERAND_ARR_ELEM:
    data = decl->data.a
               .arr[eval_expr(interpreter, &operand->idx_expr) - decl->start];
    break;
  }
  return data;
}

inline int eval_expr(Interpreter *interpreter, Expr *expr) {
  DynArr *terms = &expr->terms;
  int res = 0;
  for (int i = 0; i < terms->item_cnts; i++) {
    ExprTerm *term = da_get(terms, i);
    switch (term->typ) {
    case EXPR_TERM_CONST:
      res += term->coefficient * term->constant;
      break;
    case EXPR_TERM_OPERAND:
      res += term->coefficient * read_operand(interpreter, &term->operand);
      break;
    }
  }
  return res;
}

void execute_stmts(Interpreter *interpreter, DynArr *stmts);

char execute_cond(Interpreter *interpreter, Cond *cond) {
  int left = eval_expr(interpreter, &cond->left);
  int right = eval_expr(interpreter, &cond->right);
  switch (cond->typ) {
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
      write_operand(interpreter, &hor->var, i);
      execute_stmts(interpreter, &hor->stmts);
    }
    // apply end
    write_operand(interpreter, &hor->var, end);
  } break;
  case STMT_YOSORO_CMD: {
    YosoroStmt *yosoro = &stmt->inner.yosoro;
    int res = eval_expr(interpreter, &yosoro->expr);
    printf("%d ", res);
  } break;
  case STMT_SET_CMD: {
    SetStmt *set = &stmt->inner.set;
    int res = eval_expr(interpreter, &set->expr);
    write_operand(interpreter, &set->operand, res);
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
