#ifndef _PARSER_H_
#define _PARSER_H_

#include "interpreter.h"
#include "lexer.h"
#include "utils.h"

#ifndef NO_STD_INC
#include <stddef.h>
#endif

#define CMD_PREFIX 0xf0
#define BLK_PREFIX 0x00

enum OperandTyp {
  OPERAND_INT_VAR,
  OPERAND_ARR_ELEM,
};

enum ExprTermTyp {
  EXPR_TERM_CONST,
  EXPR_TERM_OPERAND, // Something can be operand
};

typedef struct Expr {
  DynArr terms; // ExprTerm *
} Expr;         // Use Flatten Expression

typedef struct VarDecl VarDecl;

typedef struct Operand {
  enum OperandTyp typ;
  size_t decl_idx;
  union {
    Expr idx_expr; // for ArrElem
  };
} Operand;

typedef struct ExprTerm {
  int coefficient;
  enum ExprTermTyp typ;
  union {
    int constant;
    Operand operand;
  };
} ExprTerm;

enum StmtType {
  // STMT_VARS_BLK, Moved to Parser
  STMT_IHU_BLK = BLK_PREFIX,
  STMT_WHILE_BLK,
  STMT_HOR_BLK,
  STMT_YOSORO_CMD = CMD_PREFIX,
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
    size_t start;
    size_t end;
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
