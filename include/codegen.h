#ifdef CODEGEN

#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#ifndef NO_CUSTOM_INC
#include "parser.h"
#include "utils.h"
#endif

enum OpCodeType {
#undef OPCODE
#define OPCODE(x) x,
#include "opcodes.h"
#undef OPCODE
};

typedef struct CondJmp {
  enum CmpType cmp_typ;
  short offset;
} CondJmp;

typedef struct IncVar {
  VarDecl *ptr;
  int constant;
} VarConst;

typedef struct OpCode {
  enum OpCodeType typ;
  union OpCodeData {
    short offset;
    int constant;
    int term_cnts;
    VarDecl *ptr;
    CondJmp cjmp;
    VarConst var_const;
    // enum CmpType cmp_typ;
  } data;
} OpCode;

typedef struct CodeGen {
  DynArr codes;
  DynArr *stmts;
  DynArr *var_decls;
} CodeGen;

void cg_init(CodeGen *cg, DynArr *stmts, DynArr *var_decls);
CodeGen *cg_create(DynArr *stmts, DynArr *var_decls);
void cg_free(CodeGen *cg);
void cg_gen(CodeGen *cg);
#ifndef NO_DEBUG
const char *op_code_type(enum OpCodeType op_typ);
void cg_debug(CodeGen *cg);
#endif

#endif // _CODEGEN_H_

#endif