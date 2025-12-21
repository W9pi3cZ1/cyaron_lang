#ifndef _PARSER_H_
#define _PARSER_H_

#pragma once

#ifndef NO_CUSTOM_INC
#include "interpreter.h"
#include "lexer.h"
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
} Expr;         // Use Flatten Expression

typedef struct VarDecl VarDecl;

typedef struct Operand {
  enum OperandTyp typ;
  int decl_idx;
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

typedef struct VarDecl {
  enum VarType typ;
  // union {
  VarData data;
  const char *name;
  // };
  struct {
    int start;
    int end;
  };
} VarDecl;

typedef struct Cond {
  enum TokType typ; // TOK_CMP_[XXX]
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

Parser *parser_create(DynArr *toks);
void parser_free(Parser *parser);
size_t operand_finder(Parser *parser, const char *var_name,
                      enum OperandTyp type);
DynArr *parser_parse(Parser *parser);
#ifndef NO_DEBUG
void debug_parser(Parser *parser);
#endif

#endif // _PARSER_H_
