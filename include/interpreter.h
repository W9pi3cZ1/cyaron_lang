#ifndef CODEGEN

#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#pragma once

#ifndef NO_CUSTOM_INC
#include "utils.h"
#endif

#ifndef NO_STD_INC
#include <stddef.h>
#endif

typedef struct Interpreter {
  DynArr *stmts; // Stmt *
  // usize exec_ptr;
  DynArr *var_decls; // VarDecl<VarData> *
#ifndef NO_DEBUG
  struct InterpreterStats {
    usize operand_read;
    usize operand_write;
    usize expression_eval;
  } stats;
#endif
} Interpreter;

void interpreter_init(Interpreter *interpreter, DynArr *stmts,
                      DynArr *var_decls);
Interpreter *interpreter_create(DynArr *stmts, DynArr *var_decls);
void interpreter_free(Interpreter *interpreter);
void interpreter_execute(Interpreter *interpreter);
#ifndef NO_DEBUG
void interpreter_stats(Interpreter *interpreter);
#endif

#endif // _INTERPRETER_H_

#endif