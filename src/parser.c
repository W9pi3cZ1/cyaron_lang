#include "parser.h"
#include "lexer.h"
#include "utils.h"

#ifndef NO_STD_INC
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

Parser *parser_create(DynArr *toks) {
  Parser *parser = malloc(sizeof(Parser));
  parser->toks = toks;
  parser->pos = 0;
  da_init(&parser->stmts, sizeof(Stmt), 16);
  da_init(&parser->var_decls, sizeof(VarDecl), 16);
  return parser;
}

void free_operand(Operand *op);

void free_expr(Expr *expr) {
  for (int i = 0; i < expr->terms.item_cnts; i++) {
    ExprTerm *term = da_get(&expr->terms, i);
    switch (term->typ) {
    case EXPR_TERM_CONST:
      continue;
    case EXPR_TERM_OPERAND:
      free_operand(&term->operand);
      break;
    }
  }
  da_free(&expr->terms);
}

void free_operand(Operand *op) {
  if (op->typ == OPERAND_ARR_ELEM) {
    free_expr(&op->idx_expr);
  }
}

void free_stmts(DynArr *stmts) {
  for (int i = 0; i < stmts->item_cnts; i++) {
    Stmt *stmt = da_get(stmts, i);
    switch (stmt->typ) {
    case STMT_YOSORO_CMD:
      free_expr(&stmt->inner.yosoro.expr);
      break;
    case STMT_SET_CMD:
      free_operand(&stmt->inner.set.operand);
      free_expr(&stmt->inner.set.expr);
      break;
    case STMT_IHU_BLK:
      free_expr(&stmt->inner.ihu.cond.left);
      free_expr(&stmt->inner.ihu.cond.right);
      free_stmts(&stmt->inner.ihu.stmts);
      break;
    case STMT_WHILE_BLK:
      free_expr(&stmt->inner.while_stmt.cond.left);
      free_expr(&stmt->inner.while_stmt.cond.right);
      free_stmts(&stmt->inner.while_stmt.stmts);
      break;
    case STMT_HOR_BLK:
      free_operand(&stmt->inner.hor.var);
      free_expr(&stmt->inner.hor.start);
      free_expr(&stmt->inner.hor.end);
      free_stmts(&stmt->inner.hor.stmts);
    }
  }
  da_free(stmts);
}

void parser_free(Parser *parser) {
  free_stmts(&parser->stmts);
  da_free(&parser->var_decls);
  free(parser);
}

static Token *current_token(Parser *parser) {
  if (parser->pos >= parser->toks->item_cnts) {
    return NULL;
  }
  return da_get(parser->toks, parser->pos);
}

static Token *peek_token(Parser *parser, size_t offset) {
  if (parser->pos + offset >= parser->toks->item_cnts) {
    return NULL;
  }
  return da_get(parser->toks, parser->pos + offset);
}

static void consume_token(Parser *parser) { parser->pos++; }

static int match_token(Parser *parser, enum TokType typ) {
  Token *token = current_token(parser);
  return token && token->type == typ;
}

static Token *next_token(Parser *parser) {
  Token *token = current_token(parser);
  //   if (!token || token->type != typ) {
  //     err_log("Syntax error: expected token type %d but %d\n", typ,
  //             token ? (int)token->type : -1);
  //     exit(1);
  //   }
  consume_token(parser);
  return token;
}

void parse_vars(Parser *parser) {
  consume_token(parser); // {
  consume_token(parser); // vars
  DynArr *var_decls = &parser->var_decls;
  size_t cnt = 0;
  while (!match_token(parser, TOK_RBRACE)) {
    Token *ident = next_token(parser); // IDENT
    consume_token(parser);             // :
    VarDecl *decl = da_try_push_back(var_decls);
    decl->name = ident->str;
    if (match_token(parser, TOK_KEYWORD_INT)) {
      consume_token(parser); // int
      decl->typ = VAR_INT;
    } else if (match_token(parser, TOK_KEYWORD_ARRAY)) {
      consume_token(parser); // array
      consume_token(parser); // [
      consume_token(parser); // int
      consume_token(parser); // ,
      decl->typ = VAR_ARR;
      decl->start = atoll(next_token(parser)->str);
      consume_token(parser); // ..
      decl->end = atoll(next_token(parser)->str);
      consume_token(parser); // ]
    } // else ERR!
    cnt++;
  }
  consume_token(parser); // }
}

void parse_blk(Parser *parser, DynArr *stmts);
void parse_expr(Parser *parser, Expr *expr);

size_t operand_finder(Parser *parser, const char *var_name,
                      enum OperandTyp type) {
  enum VarType target_type = 0xff;
  switch (type) {
  case OPERAND_INT_VAR:
    target_type = VAR_INT;
    break;
  case OPERAND_ARR_ELEM:
    target_type = VAR_ARR;
    break;
  }
  for (int i = 0; i < parser->var_decls.item_cnts; i++) {
    VarDecl *decl = da_get(&parser->var_decls, i);
    if ((target_type == 0xff || decl->typ == target_type) &&
        decl->name == var_name
        // || strcmp(decl->name, var_name) == 0
        // strcmp is unecessary (because of StrPool)
    ) {
      return i;
    }
  }
  return (size_t)-1;
}

void parse_operand(Parser *parser, Operand *operand) {
  Token *operand_tok = current_token(parser);
  consume_token(parser); // var_name
  operand->typ = OPERAND_INT_VAR;
  if (match_token(parser, TOK_LBRACKET)) {
    operand->typ = OPERAND_ARR_ELEM;
    consume_token(parser); // [
    parse_expr(parser, &operand->idx_expr);
    consume_token(parser); // ]
  }
  operand->decl_idx = operand_finder(parser, operand_tok->str, operand->typ);
}

int is_operand_eq(Operand *a, Operand *b);

int is_expr_eq(Expr *a, Expr *b) {
  // *Completely Equal*
  int cnts = a->terms.item_cnts;
  if (cnts != b->terms.item_cnts) {
    return 0;
  }
  for (int i = 0; i < cnts; i++) {
    ExprTerm *a_term = da_get(&a->terms, i);
    ExprTerm *b_term = da_get(&b->terms, i);
    if (a_term->typ != b_term->typ) {
      return 0;
    }
    switch (a_term->typ) {
    case EXPR_TERM_CONST:
      return a_term->constant == b_term->constant;
    case EXPR_TERM_OPERAND:
      return is_operand_eq(&a_term->operand, &b_term->operand);
    }
  }
  return 1;
}

int is_operand_eq(Operand *a, Operand *b) {
  if (a->typ != b->typ || a->decl_idx != b->decl_idx) {
    return 0;
  }
  if (a->typ == OPERAND_INT_VAR) {
    return 1;
  } else { // ARR_ELEM
    return is_expr_eq(&a->idx_expr, &b->idx_expr);
  }
}

void terms_add_operand(DynArr *terms, Operand op, int sign) {
  for (int i = 0; i < terms->item_cnts; i++) {
    ExprTerm *term = da_get(terms, i);
    if (term->typ == EXPR_TERM_OPERAND && is_operand_eq(&term->operand, &op)) {
      term->coefficient += sign;
      if (op.typ == OPERAND_ARR_ELEM) {
        da_free(&op.idx_expr.terms);
      }
      return;
    }
  }
  ExprTerm *term = da_try_push_back(terms);
  term->coefficient = sign;
  term->typ = EXPR_TERM_OPERAND;
  term->operand = op;
  return;
}

void parse_expr(Parser *parser, Expr *expr) {
  da_init(&expr->terms, sizeof(ExprTerm), 8);
  DynArr *terms = &expr->terms;
  int const_val = 0;
  short sign = 1;

  while (current_token(parser) &&
         !(match_token(parser, TOK_EOF) || match_token(parser, TOK_COLON) ||
           match_token(parser, TOK_COMMA) || match_token(parser, TOK_RBRACE) ||
           match_token(parser, TOK_LBRACE) ||
           match_token(parser, TOK_RBRACKET))) {
    sign = 1;
    if (match_token(parser, TOK_PLUS)) {
      consume_token(parser);
    } else if (match_token(parser, TOK_MINUS)) {
      sign = -1;
      consume_token(parser);
    }

    Token *term_tok = current_token(parser);
    if (term_tok->type == TOK_INT) {
      const_val += atoi(term_tok->str) * sign;
      consume_token(parser);
      continue;
    }
    Operand op;
    parse_operand(parser, &op);
    terms_add_operand(terms, op, sign);
  }
  if (const_val != 0 || terms->item_cnts == 0) {
    ExprTerm *const_term = da_try_push_back(terms);
    const_term->coefficient = 1;
    const_term->typ = EXPR_TERM_CONST;
    const_term->constant = const_val;
  }
}

void parse_cond(Parser *parser, Cond *cond) {
  cond->typ = next_token(parser)->type; // EQ, NEQ...
  consume_token(parser);                // ,
  parse_expr(parser, &cond->left);
  consume_token(parser); // ,
  parse_expr(parser, &cond->right);
}

void parse_ihu(Parser *parser, Stmt *stmt) {
  consume_token(parser); // {
  consume_token(parser); // ihu
  stmt->typ = STMT_IHU_BLK;
  IhuStmt *ihu = &stmt->inner.ihu;
  parse_cond(parser, &ihu->cond);
  da_init(&ihu->stmts, sizeof(Stmt), 16);
  parse_blk(parser, &ihu->stmts);
  consume_token(parser); // }
}
void parse_while(Parser *parser, Stmt *stmt) {
  consume_token(parser); // {
  consume_token(parser); // while
  stmt->typ = STMT_WHILE_BLK;
  WhileStmt *while_stmt = &stmt->inner.while_stmt;
  parse_cond(parser, &while_stmt->cond);
  da_init(&while_stmt->stmts, sizeof(Stmt), 16);
  parse_blk(parser, &while_stmt->stmts);
  consume_token(parser); // }
}

void parse_hor(Parser *parser, Stmt *stmt) {
  consume_token(parser); // {
  consume_token(parser); // hor
  stmt->typ = STMT_HOR_BLK;
  HorStmt *hor = &stmt->inner.hor;
  parse_operand(parser, &hor->var);
  consume_token(parser); // ,
  parse_expr(parser, &hor->start);
  consume_token(parser); // ,
  parse_expr(parser, &hor->end);
  da_init(&hor->stmts, sizeof(Stmt), 16);
  parse_blk(parser, &hor->stmts);
  consume_token(parser); // }
}

void parse_yosoro(Parser *parser, Stmt *stmt) {
  consume_token(parser); // :
  consume_token(parser); // yosoro
  stmt->typ = STMT_YOSORO_CMD;
  YosoroStmt *yosoro = &stmt->inner.yosoro;
  parse_expr(parser, &yosoro->expr);
}

void parse_set(Parser *parser, Stmt *stmt) {
  consume_token(parser); // :
  consume_token(parser); // set
  stmt->typ = STMT_SET_CMD;
  SetStmt *set = &stmt->inner.set;
  parse_operand(parser, &set->operand);
  consume_token(parser); // ,
  parse_expr(parser, &set->expr);
}

static void parse_stmts(Parser *parser, DynArr *stmts) {
  Token *tok = current_token(parser);
  if (!tok) {
    return;
  }
  Stmt *cur_stmt;
  switch (tok->type) {
  case TOK_LBRACE:
    tok = peek_token(parser, 1);
    if (!tok) {
      consume_token(parser);
      return;
    }
    switch (tok->type) {
    case TOK_KEYWORD_VARS:
      parse_vars(parser);
      break;
    case TOK_KEYWORD_IHU:
      cur_stmt = da_try_push_back(stmts);
      parse_ihu(parser, cur_stmt);
      break;
    case TOK_KEYWORD_WHILE:
      cur_stmt = da_try_push_back(stmts);
      parse_while(parser, cur_stmt);
      break;
    case TOK_KEYWORD_HOR:
      cur_stmt = da_try_push_back(stmts);
      parse_hor(parser, cur_stmt);
      break;
    default:
      break;
    }
    return;
  case TOK_COLON:
    tok = peek_token(parser, 1);
    if (!tok) {
      consume_token(parser);
      return;
    }
    cur_stmt = da_try_push_back(stmts);
    switch (tok->type) {
    case TOK_KEYWORD_YOSORO:
      parse_yosoro(parser, cur_stmt);
      break;
    case TOK_KEYWORD_SET:
      parse_set(parser, cur_stmt);
      break;
    default:
      break;
    }
    break;
  default:
    consume_token(parser);
    return;
  }
}

void parse_blk(Parser *parser, DynArr *stmts) {
  while (!match_token(parser, TOK_RBRACE) &&
         parser->pos < parser->toks->item_cnts &&
         !match_token(parser, TOK_EOF)) {
    parse_stmts(parser, stmts);
  }
}

DynArr *parser_parse(Parser *parser) {
  while (parser->pos < parser->toks->item_cnts &&
         !match_token(parser, TOK_EOF)) {
    parse_stmts(parser, &parser->stmts);
  }
  return &parser->stmts;
}

#ifndef NO_DEBUG
static void printf_indent(int indent, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  for (int i = 0; i < indent; i++) {
    printf("    ");
  }
  vprintf(fmt, ap);
  va_end(ap);
}

void debug_expr(Expr *expr);

void debug_operand(Operand *operand) {
  printf("#%zu", operand->decl_idx);
  switch (operand->typ) {
  case OPERAND_INT_VAR:
    break;
  case OPERAND_ARR_ELEM:
    printf("[");
    debug_expr(&operand->idx_expr);
    printf("]");
    break;
  }
}

void debug_expr(Expr *expr) {
  DynArr *terms = &expr->terms;
  for (int i = 0; i < terms->item_cnts; i++) {
    ExprTerm *term = da_get(terms, i);
    // if (term->coefficient == -1) {
    //   printf("-");
    // } else if (i > 0) {
    //   printf("+");
    // }
    printf("%+d*", term->coefficient);
    switch (term->typ) {
    case EXPR_TERM_CONST:
      printf("%d", term->constant);
      break;
    case EXPR_TERM_OPERAND:
      debug_operand(&term->operand);
      break;
    }
  }
}

void debug_stmt(Stmt *stmt, int indent);

void debug_blk(DynArr *stmts, int indent) {
  for (int i = 0; i < stmts->item_cnts; i++) {
    Stmt *stmt = da_get(stmts, i);
    debug_stmt(stmt, indent);
  }
}

void debug_cond(Cond *cond) {
  printf("%s", debug_token_type(cond->typ));
  printf(",");
  debug_expr(&cond->left);
  printf(",");
  debug_expr(&cond->right);
}

void debug_stmt(Stmt *stmt, int indent) {
  if (!stmt) {
    return;
  }
  switch (stmt->typ) {
  case STMT_IHU_BLK: {
    IhuStmt *ihu = &stmt->inner.ihu;
    printf_indent(indent, "If(");
    debug_cond(&ihu->cond);
    printf("){\n");
    debug_blk(&ihu->stmts, indent + 1);
    printf_indent(indent, "}\n");
  } break;
  case STMT_WHILE_BLK: {
    WhileStmt *while_stmt = &stmt->inner.while_stmt;
    printf_indent(indent, "While(");
    debug_cond(&while_stmt->cond);
    printf("){\n");
    debug_blk(&while_stmt->stmts, indent + 1);
    printf_indent(indent, "}\n");
  } break;
  case STMT_HOR_BLK: {
    HorStmt *hor = &stmt->inner.hor;
    printf_indent(indent, "Hor(");
    debug_operand(&hor->var);
    printf(",");
    debug_expr(&hor->start);
    printf(",");
    debug_expr(&hor->end);
    printf("){\n");
    debug_blk(&hor->stmts, indent + 1);
    printf_indent(indent, "}\n");
  } break;
  case STMT_YOSORO_CMD: {
    YosoroStmt *yosoro = &stmt->inner.yosoro;
    printf_indent(indent, "Yosoro(");
    debug_expr(&yosoro->expr);
    printf(")\n");
  } break;
  case STMT_SET_CMD: {
    SetStmt *set = &stmt->inner.set;
    printf_indent(indent, "Set(");
    debug_operand(&set->operand);
    printf(",");
    debug_expr(&set->expr);
    printf(")\n");
  } break;
  }
}

void debug_parser(Parser *parser) {
  printf_indent(0, "Vars{\n");
  DynArr *var_decls = &parser->var_decls;
  for (int i = 0; i < var_decls->item_cnts; i++) {
    VarDecl *decl = da_get(var_decls, i);
    switch (decl->typ) {
    case VAR_INT:
      printf_indent(1, "%s(#%d):int,\n", decl->name, i);
      break;
    case VAR_ARR:
      printf_indent(1, "%s(#%d):array[int, %zu..%zu],\n", decl->name, i,
                    decl->start, decl->end);
      break;
    }
  }
  printf_indent(0, "}\n");
  debug_blk(&parser->stmts, 0);
  return;
}
#endif
