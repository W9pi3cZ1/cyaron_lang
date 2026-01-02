#ifndef NO_CUSTOM_INC
#include "parser.h"
#include "lexer.h"
#include "utils.h"
#endif

#ifndef NO_STD_INC
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

void parser_init(Parser *parser, DynArr *toks) {
  memset(parser, 0, sizeof(Parser));
  parser->toks = toks;
  da_init(&parser->stmts, sizeof(Stmt), 16);
  da_init(&parser->var_decls, sizeof(VarDecl), 50);
}

Parser *parser_create(DynArr *toks) {
  Parser *parser = malloc(sizeof(Parser));
  parser_init(parser, toks);
  return parser;
}

void free_operand(Operand *op);

void free_expr(Expr *expr) {
  OperandTerm *op_term = expr->op_terms.items;
  for (int i = 0; i < expr->op_terms.item_cnts; i++, op_term++) {
    free_operand(&op_term->operand);
  }
  da_free(&expr->op_terms);
}

void free_operand(Operand *op) {
  if (op->typ == OPERAND_ARR_ELEM) {
    free_expr(&op->idx_expr);
  }
}

void free_stmts(DynArr *stmts) {
  Stmt *stmt = stmts->items;
  for (int i = 0; i < stmts->item_cnts; i++, stmt++) {
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
  VarDecl *var_decl = parser->var_decls.items;
  for (int i = 0; i < parser->var_decls.item_cnts; i++, var_decl++) {
    if (var_decl->typ == VAR_ARR) {
      VarArrData *arr_data = &var_decl->data.a;
      free(arr_data->arr);
    }
  }
  free_stmts(&parser->stmts);
  da_free(&parser->var_decls);
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
  while (!match_token(parser, TOK_RBRACE)) {
    Token *ident = next_token(parser); // IDENT
    consume_token(parser);             // :
    VarDecl *decl = da_try_push_back(var_decls);
    decl->name = ident->str;
    decl->decl_idx = var_decls->item_cnts - 1;
    if (match_token(parser, TOK_KEYWORD_INT)) {
      consume_token(parser); // int
      decl->typ = VAR_INT;
      decl->data.i.val = 0;
    } else
    //  if (match_token(parser, TOK_KEYWORD_ARRAY))
    {
      consume_token(parser); // array
      consume_token(parser); // [
      consume_token(parser); // int
      consume_token(parser); // ,
      decl->typ = VAR_ARR;
      decl->start = atoi(next_token(parser)->str);
      consume_token(parser); // ..
      decl->end = atoi(next_token(parser)->str);
      decl->data.a.arr = calloc(decl->end - decl->start + 1, sizeof(int));
      consume_token(parser); // ]
    } // else ERR!
  }
  consume_token(parser); // }
}

void parse_blk(Parser *parser, DynArr *stmts);
void parse_expr(Parser *parser, Expr *expr);

unsigned short operand_finder(Parser *parser, const char *var_name,
                              enum OperandTyp type) {
  VarDecl *decl = parser->var_decls.items;
  for (int i = 0; i < parser->var_decls.item_cnts; i++, decl++) {
    if (decl->name == var_name
        // || strcmp(decl->name, var_name) == 0
        // strcmp is unecessary (because of StrPool)
        // warn: without type checker
    ) {
      return i;
    }
  }
  return (unsigned short)-1;
}

void parse_operand(Parser *parser, Operand *operand) {
  Token *operand_tok = current_token(parser);
  consume_token(parser); // var_name
  // operand->decl_expand = UNEXPANDED;
  operand->typ = OPERAND_INT_VAR;
  if (match_token(parser, TOK_LBRACKET)) {
    operand->typ = OPERAND_ARR_ELEM;
    consume_token(parser); // [
    parse_expr(parser, &operand->idx_expr);
    consume_token(parser); // ]
  }
  unsigned short decl_idx =
      operand_finder(parser, operand_tok->str, operand->typ);
  operand->decl_idx = decl_idx;
  // // Maybe unsafe (when there is
  // // VARS block after this...)
  // operand->decl_ptr = operand->decl_ptr =
  //     (VarDecl *)(parser->var_decls.items) + decl_idx;
}

int is_operand_eq(Operand *a, Operand *b);

int is_expr_eq(Expr *a, Expr *b) {
  // *Completely Equal*
  int cnts = a->op_terms.item_cnts;
  if (cnts != b->op_terms.item_cnts || a->constant != b->constant) {
    return 0;
  }
  OperandTerm *a_term = a->op_terms.items;
  OperandTerm *b_term = b->op_terms.items;
  for (int i = 0; i < cnts; i++, ++a_term, ++b_term) {
    if (!is_operand_eq(&a_term->operand, &b_term->operand))
      return 0;
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

void terms_add_operand(DynArr *op_terms, Operand op, int sign) {
  OperandTerm *op_term = op_terms->items;
  for (int i = 0; i < op_terms->item_cnts; i++, op_term++) {
    if (is_operand_eq(&op_term->operand, &op)) {
      op_term->coefficient += sign;
      if (op.typ == OPERAND_ARR_ELEM) {
        da_free(&op.idx_expr.op_terms);
      }
      return;
    }
  }
  op_term = da_try_push_back(op_terms);
  op_term->coefficient = sign;
  op_term->operand = op;
  return;
}

static int is_expr_end(Parser *parser) {
  Token *cur_tok = current_token(parser);
  if (!cur_tok) {
    return 1;
  }
  switch (cur_tok->type) {
  case TOK_EOF:
  case TOK_COLON:
  case TOK_COMMA:
  case TOK_LBRACE:
  case TOK_RBRACE:
  case TOK_LBRACKET:
  case TOK_RBRACKET:
    return 1;
  default:
    return 0;
  }
}

static int compare_operand_terms(const void *a, const void *b) {
  const OperandTerm *term_a = (const OperandTerm *)a;
  const OperandTerm *term_b = (const OperandTerm *)b;
  if (term_a->coefficient < term_b->coefficient) {
    return 1;
  } else if (term_a->coefficient > term_b->coefficient) {
    return -1;
  }
  if (term_a->operand.decl_idx < term_b->operand.decl_idx) {
    return -1;
  } else if (term_a->operand.decl_idx > term_b->operand.decl_idx) {
    return 1;
  }
  return 0;
}

void optimize_expr(Parser *parser, Expr *expr) {
  if (expr->op_terms.item_cnts > 1) {
    qsort(expr->op_terms.items, expr->op_terms.item_cnts, sizeof(OperandTerm),
          compare_operand_terms);
  }
  // // cut-off terms that coefficient=0
  // OperandTerm *op_term = expr->op_terms.items;
  // int end_i = 0;
  // for (end_i = 0; end_i < expr->op_terms.item_cnts; end_i++, op_term++) {
  //   if (op_term->coefficient == 0) {
  //     expr->op_terms.item_cnts = end_i;
  //     break;
  //   }
  // }
}

void parse_expr(Parser *parser, Expr *expr) {
  da_init(&expr->op_terms, sizeof(OperandTerm), 4);
  DynArr *op_terms = &expr->op_terms;
  int const_val = 0;
  short sign = 1;

  while (!is_expr_end(parser)) {
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
    terms_add_operand(op_terms, op, sign);
  }
  expr->constant = const_val;
  optimize_expr(parser, expr);
}

void parse_cond(Parser *parser, Cond *cond) {
  cond->typ = next_token(parser)->type - TOK_CMP_LT + 1; // EQ, NEQ...

  consume_token(parser); // ,
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
  printf("#%hd", operand->decl_idx);
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
  DynArr *op_terms = &expr->op_terms;
  OperandTerm *op_term = op_terms->items;

  if (expr->constant != 0 || expr->op_terms.item_cnts == 0)
    printf("%+d", expr->constant);

  for (int i = 0; i < op_terms->item_cnts; i++, op_term++) {
    printf("%+d*", op_term->coefficient);
    debug_operand(&op_term->operand);
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
  printf("%s", debug_token_type(cond->typ + TOK_CMP_LT - 1));
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
