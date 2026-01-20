#ifndef _LEXER_H_
#define _LEXER_H_

#pragma once

#ifndef NO_CUSTOM_INC
#include "utils.h"
#endif

#ifndef NO_STD_INC
#include <stddef.h>
#endif

enum TokType {
#undef CYR_TOKS
#define CYR_TOKS(x) x,
#include "cyr_toks.h"
#undef CYR_TOKS
};

typedef struct Token {
  enum TokType type;
  const char *str;
} Token;

typedef struct Lexer {
  char *src;
  usize src_len;
  usize ch_pos;
  DynArr toks; // Always store Token(s)
  StrPool tok_val_pool;
} Lexer;

void lexer_init(Lexer *lexer, char *src);
Lexer *lexer_create(char *src);
void lexer_free(Lexer *lexer);
void token_init(Token *tok, enum TokType tok_type, const char *str);
Token *token_create(enum TokType tok_type, const char *str);
void lexer_tokenize(Lexer *lexer);
#ifndef NO_DEBUG
const char *debug_token_type(enum TokType tok_type);
void debug_lexer(Lexer *lexer);
#endif
#endif // _LEXER_H_
