#include "lexer.h"
#include "utils.h"

#ifndef NO_STD_INC
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifndef NO_DEBUG
const char *debug_token_type(enum TokType tok_type) {
  switch (tok_type) {
#undef CYR_TOKS
#define CYR_TOKS(x)                                                            \
  case x:                                                                      \
    return #x;
#include "cyr_toks.def"
#undef CYR_TOKS
  default:
    return "<Invalid>";
  }
}
#endif

Lexer *lexer_create(char *src) {
  Lexer *lexer = malloc(sizeof(Lexer));
  str_pool_init(&lexer->tok_val_pool, 512, 16);
  lexer->src_len = strlen(src);
  lexer->src = src;
  // lexer->src = malloc(lexer->src_len);
  // memcpy(lexer->src, src, lexer->src_len); // That's a copy!! :)
  lexer->ch_pos = 0;
  da_init(&lexer->toks, sizeof(Token), 128);
  return lexer;
}

void lexer_free(Lexer *lexer) {
  // free token list
  da_free(&lexer->toks);
  str_pool_free(&lexer->tok_val_pool);
  free(lexer);
}

void token_init(Token *tok, enum TokType tok_type, const char *str) {
  // if (tok == NULL) {
  //   err_log("Argument is a Null Pointer (in %s)\n", __FUNCTION__);
  // }
  tok->type = tok_type;
  tok->str = str;
  return;
}

Token *token_create(enum TokType tok_type, const char *str) {
  Token *tok = malloc(sizeof(Token));
  // if (tok == NULL) {
  //   err_log("Failed to allocate for token\n");
  // }
  token_init(tok, tok_type, str);
  return tok;
}

inline static void add_token(Lexer *lexer, enum TokType tok_type,
                             const char *val) {
  Token *tok = da_try_push_back(&lexer->toks);
  token_init(tok, tok_type, val);
  return;
}

inline static char is_identifier_char(char c) { return isalpha(c) || c == '_'; }

#define KEYWORD(typ, val) {typ, val, sizeof(val) - 1}

const struct KeyWords {
  const enum TokType typ;
  const char *val;
  const size_t len;
} keywords[] = {
    KEYWORD(TOK_KEYWORD_VARS, "vars"),
    KEYWORD(TOK_KEYWORD_SET, "set"),
    KEYWORD(TOK_KEYWORD_YOSORO, "yosoro"),
    KEYWORD(TOK_KEYWORD_IHU, "ihu"),
    KEYWORD(TOK_KEYWORD_HOR, "hor"),
    KEYWORD(TOK_KEYWORD_WHILE, "while"),
    KEYWORD(TOK_KEYWORD_INT, "int"),
    KEYWORD(TOK_KEYWORD_ARRAY, "array"),
    KEYWORD(TOK_CMP_LT, "lt"),
    KEYWORD(TOK_CMP_GT, "gt"),
    KEYWORD(TOK_CMP_LE, "le"),
    KEYWORD(TOK_CMP_GE, "ge"),
    KEYWORD(TOK_CMP_EQ, "eq"),
    KEYWORD(TOK_CMP_NEQ, "neq"),
};

const size_t keyword_cnts = sizeof(keywords) / sizeof(*keywords);

int check_keyword(const char *tok_val, size_t len) {
  for (int i = 0; i < keyword_cnts; ++i) {
    if (keywords[i].len == len && strncmp(tok_val, keywords[i].val, len) == 0) {
      return i;
    }
  }
  return -1;
}

void lexer_tokenize(Lexer *lexer) {
  while (lexer->ch_pos < lexer->src_len) {
    char ch = lexer->src[lexer->ch_pos];
    // Ignore blank token
    if (isspace(ch)) {
      lexer->ch_pos++;
      continue;
    }

    if (isdigit(ch)) {
      // Number literal
      size_t start = lexer->ch_pos;
      while (lexer->ch_pos < lexer->src_len &&
             isdigit(lexer->src[lexer->ch_pos])) {
        lexer->ch_pos++;
      }
      size_t len = lexer->ch_pos - start;
      char *val = lexer->src + start;
      val = str_pool_intern(&lexer->tok_val_pool, val, len);
      add_token(lexer, TOK_INT, val);
      continue;
    }

    if (is_identifier_char(ch)) {
      // Identifier or Keyword
      size_t start = lexer->ch_pos;
      while (lexer->ch_pos < lexer->src_len &&
             isalnum(lexer->src[lexer->ch_pos])) {
        lexer->ch_pos++;
      }
      size_t len = lexer->ch_pos - start;
      const char *val = lexer->src + start;
      // Check keywords
      int keyword_idx = check_keyword(val, len);
      enum TokType type;
      if (keyword_idx != -1) {
        type = keywords[keyword_idx].typ;
        val = keywords[keyword_idx].val;
      } else {
        type = TOK_IDENT;
        val = str_pool_intern(&lexer->tok_val_pool, val, len);
      }
      add_token(lexer, type, val);
      continue;
    }

#define SCH_TOK(typ, ch)                                                       \
  case ch:                                                                     \
    add_token(lexer, typ, #ch);                                                \
    break;

    // May a single-char token
    switch (ch) {
      SCH_TOK(TOK_PLUS, '+')
      SCH_TOK(TOK_MINUS, '-')
      SCH_TOK(TOK_COMMA, ',')
      SCH_TOK(TOK_COLON, ':')
      SCH_TOK(TOK_LBRACE, '{')
      SCH_TOK(TOK_RBRACE, '}')
      SCH_TOK(TOK_LBRACKET, '[')
      SCH_TOK(TOK_RBRACKET, ']')
    default:
      // Double dot(..)
      if (ch == '.' && lexer->ch_pos + 1 < lexer->src_len &&
          lexer->src[lexer->ch_pos + 1] == '.') {
        add_token(lexer, TOK_DOTDOT, "..");
        lexer->ch_pos += 2;
        continue;
      } else {
        // Unknown
        lexer->ch_pos++;
        continue;
      }
      break;
    }
    lexer->ch_pos++;
  }
  add_token(lexer, TOK_EOF, NULL);
  return;
}

#ifndef NO_DEBUG
void debug_lexer(Lexer *lexer) {
  logger("Lexer{\n"
         "    ch_pos: %zu,\n"
         "    src: \"\n%s\n\",\n"
         "    src_len: %zu,\n"
         "    toks.arr: [\n",
         lexer->ch_pos, lexer->src, lexer->src_len);
  Token *tok_ptr = NULL;
  for (size_t i = 0; i < lexer->toks.item_cnts; i++) {
    tok_ptr = &((Token *)lexer->toks.items)[i];
    logger("        %s(%s),\n", debug_token_type(tok_ptr->type), tok_ptr->str);
  }
  logger("    ](%zu items ...),\n"
         "}\n\n",
         lexer->toks.item_cnts);
  debug_str_pool(&lexer->tok_val_pool);
}
#endif
