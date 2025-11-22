#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "utils.h"
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
} Interpreter;

Interpreter *interpreter_create(DynArr *stmts, DynArr *var_decls);
void interpreter_free(Interpreter *interpreter);
void interpreter_execute(Interpreter *interpreter);

#endif // _INTERPRETER_H_
