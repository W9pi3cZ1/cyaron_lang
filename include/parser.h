#ifndef _PARSER_H_
#define _PARSER_H_

#pragma once

#ifndef NO_CUSTOM_INC
#include "utils.h"
#endif

#ifndef NO_STD_INC
#include <stddef.h>
#endif

enum OperandTyp {
  OPERAND_INT_VAR,
  OPERAND_ARR_ELEM,
};

typedef struct Expr {
  DynArr op_terms; // OperandTerm *
  int constant;
} Expr; // Use Flatten Expression

typedef struct VarDecl VarDecl;

typedef struct Operand {
  enum OperandTyp typ;
  unsigned short decl_idx;
  union {
    Expr idx_expr; // for ArrElem
  };
} Operand;

typedef struct OperandTerm {
  int coefficient;
  Operand operand;
} OperandTerm;

enum StmtType {
  // STMT_VARS_BLK, Moved to Parser
  STMT_IHU_BLK,
  STMT_WHILE_BLK,
  STMT_HOR_BLK,
  STMT_YOSORO_CMD,
  STMT_SET_CMD,
};

enum VarType {
  VAR_INT,
  VAR_ARR,
};

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

typedef struct VarDecl {
  enum VarType typ;
  // union {
  VarData data;
  const char *name;
  unsigned short decl_idx;
  // };
  struct {
    int start;
    int end;
  };
} VarDecl;

enum CmpType {
  CMP_LT = 0b001,
  CMP_EQ = 0b010,
  CMP_LE = 0b011,
  CMP_GT = 0b100,
  CMP_NEQ = 0b101,
  CMP_GE = 0b110,
};

char do_cmp(enum CmpType cond_typ, int left, int right);

typedef struct Cond {
  enum CmpType typ; // TOK_CMP_[XXX]
  Expr left;
  Expr right;
} Cond;

typedef struct IhuStmt {
  Cond cond;
  DynArr stmts; // Stmt *
} IhuStmt;

typedef struct WhileStmt {
  Cond cond;
  DynArr stmts; // Stmt *
} WhileStmt;

typedef struct HorStmt {
  Operand var;
  Expr start;
  Expr end;
  DynArr stmts; // Stmt *
} HorStmt;

typedef struct YosoroStmt {
  Expr expr;
} YosoroStmt;

typedef struct SetStmt {
  Operand operand;
  Expr expr;
} SetStmt;

typedef struct Stmt {
  enum StmtType typ;
  union {
    IhuStmt ihu;
    WhileStmt while_stmt;
    HorStmt hor;
    YosoroStmt yosoro;
    SetStmt set;
  } inner;
} Stmt;

typedef struct Parser {
  DynArr *toks;     // Token *
  size_t pos;       // position about the currnet the token
  DynArr stmts;     // Stmt *
  DynArr var_decls; // VarDecl *
} Parser;

void parser_init(Parser *parser, DynArr *toks);
Parser *parser_create(DynArr *toks);
void parser_free(Parser *parser);
unsigned short operand_finder(Parser *parser, const char *var_name,
                              enum OperandTyp type);
DynArr *parser_parse(Parser *parser);
#ifndef NO_DEBUG
void debug_parser(Parser *parser);
#endif

#endif // _PARSER_H_
