#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#pragma once

#ifndef NO_CUSTOM_INC
#include "utils.h"
#endif

#ifndef NO_STD_INC
#include <stddef.h>
#endif

typedef struct VarIntData {
  int val;
} VarIntData;

typedef struct VarArrData {
  int *arr;
} VarArrData;

typedef union VarData {
  VarIntData i;
  VarArrData a;
} VarData;

typedef struct Interpreter {
  DynArr *stmts; // Stmt *
  // size_t exec_ptr;
  DynArr *var_decls; // VarDecl<VarData> *
  #ifndef NO_DEBUG
  struct InterpreterStats {
    size_t operand_read;
    size_t operand_write;
    size_t expression_eval;
  } stats;
  #endif
} Interpreter;

void var_decls_init_data(DynArr *var_decls);
void var_decls_free_data(DynArr *var_decls);
void interpreter_init(Interpreter *interpreter,DynArr *stmts, DynArr *var_decls);
Interpreter *interpreter_create(DynArr *stmts, DynArr *var_decls);
void interpreter_free(Interpreter *interpreter);
void interpreter_execute(Interpreter *interpreter);
#ifndef NO_DEBUG
void interpreter_stats(Interpreter *interpreter);
#endif

#endif // _INTERPRETER_H_
