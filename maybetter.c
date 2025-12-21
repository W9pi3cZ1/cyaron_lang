/* Code from [Luogu R225422243](https://www.luogu.com.cn/record/225422243) */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#define INBUF_SIZE (1 << 12)
#define OUTBUF_SIZE (1 << 5)
#define TOKEN_POOL_SIZE (1 << 9)

#define TOK_NONE (0)
#define TOK_LBRACE (1)
#define TOK_RBRACE (2)
#define TOK_LBRACKET (3)
#define TOK_RBRACKET (4)
#define TOK_COMMA (5)
#define TOK_COLON (6)
#define TOK_2DOT (7)
#define TOK_KEYWORD (8)
#define TOK_IDENT (9)
#define TOK_NUM (10)
#define TOK_ADD (11)
#define TOK_SUB (12)

#define KEYWORD_NONE (0)
#define KEYWORD_SET (1)
#define KEYWORD_YOSORO (2)
#define KEYWORD_VARS (3)
#define KEYWORD_IHU (4)
#define KEYWORD_HOR (5)
#define KEYWORD_WHILE (6)
typedef struct __node {
  struct __node *next;
  uint16_t type;
  union {
    uint8_t keyword;
    char *ident;
    int32_t num;
    struct __node *brace_ref;
  };
} token_t;

typedef struct {
  char *name;
  int is_array;
  union {
    int32_t num;
    struct {
      int32_t base;
      int32_t *data;
    } array;
  };
} variable_t;

#define CMP_LT (0)
#define CMP_GT (1)
#define CMP_LE (2)
#define CMP_GE (3)
#define CMP_EQ (4)
#define CMP_NEQ (5)

typedef char stringtable_entry_t[64];

char inbuf[INBUF_SIZE];
char outbuf[OUTBUF_SIZE];
token_t token_pool[TOKEN_POOL_SIZE] = {0};
stringtable_entry_t string_table[33] = {"set", "yosoro", "vars", "ihu",
                                        "hor", "while",  "int",  "array"};
variable_t variable_table[50] = {0};

char *inptr = inbuf;
int inbuf_remain = 0;

int outbuf_pos = 0;

int token_pool_idx = 0;

int string_table_idx = 8;

int variable_table_idx = 0;

static inline void fill_inbuf() {
  inbuf_remain = read(0, inbuf, INBUF_SIZE);
  inptr = inbuf;
}

static inline char gc() {
  // if (inbuf_remain <= 0) fill_inbuf();
  return (inbuf_remain <= 0) ? -1 : *inptr++;
}

static inline char pc() {
  // if (inbuf_remain <= 0) fill_inbuf();
  return (inbuf_remain <= 0) ? -1 : *inptr;
}

static inline void sc(int n) {
  // while (n > inbuf_remain) {
  //   n -= inbuf_remain;
  //   fill_inbuf();
  // }

  inptr += n;
  inbuf_remain -= n;
}

static inline int32_t read_int() {
  char c = pc();
  int32_t n = 0;

  while (isdigit(c)) {
    sc(1);
    n = n * 10 + c - '0';
    c = pc();
  }

  return n;
}

static inline void flush_output() {
  if (outbuf_pos > 0) {
    write(1, outbuf, outbuf_pos);
    outbuf_pos = 0;
  }
}

static inline void print_int(int32_t n) {
  // if (outbuf_pos > OUTBUF_SIZE - 12) flush_output();

  char buf[12];
  int i = 11;
  int neg = n < 0;
  buf[11] = ' '; // 空格分隔符

  if (neg)
    n = -n;

  do {
    buf[--i] = '0' + (n % 10);
    n /= 10;
  } while (n);

  if (neg)
    buf[--i] = '-';

  int len = 12 - i;
  memcpy(outbuf + outbuf_pos, buf + i, len);
  outbuf_pos += len;
}

token_t *alloc_token() {
  // if (token_pool_idx >= TOKEN_POOL_SIZE) abort();
  return &token_pool[token_pool_idx++];
}

uint8_t classify_char(char c) {
  switch (c) {
  case '{':
    return TOK_LBRACE;
  case '}':
    return TOK_RBRACE;
  case '[':
    return TOK_LBRACKET;
  case ']':
    return TOK_RBRACKET;
  case ',':
    return TOK_COMMA;
  case ':':
    return TOK_COLON;
  case '.':
    return TOK_2DOT;
  case '+':
    return TOK_ADD;
  case '-':
    return TOK_SUB;
  }

  if (isalpha(c))
    return TOK_IDENT;
  if (isdigit(c))
    return TOK_NUM;

  return TOK_NONE;
}

char *st_get(char *str) {
  for (int i = 0; i < string_table_idx; i++) {
    if (string_table[i] == str || strcmp(string_table[i], str) == 0)
      return string_table[i];
  }

  strcpy(string_table[string_table_idx], str);
  return string_table[string_table_idx++];
}

token_t *tokenize() {
  token_t *root = alloc_token();
  token_t *node = root;
  token_t *prev = NULL;

  token_t *lbrace_refstack[16] = {0};
  int lbrace_refstack_idx = 0;

  char c = 0;
  char sb_buf[60] = {0};
  int sb_len = 0;

  while ((c = pc()) > 0) {
    uint8_t char_type = classify_char(c);
    uint8_t is_char_empty = char_type == TOK_NONE;

    if (node->type == TOK_NONE) {
      node->type = char_type;
    }

    if (is_char_empty || char_type != node->type) {
      if (is_char_empty) {
        sc(1);
        if (node->type == TOK_NONE)
          continue;
      }

      if (node->type == TOK_IDENT) {
        sb_buf[sb_len] = '\0';
        node->ident = st_get(sb_buf);
        sb_len = 0;

        if (node->ident == string_table[KEYWORD_SET - 1]) {
          prev->type = TOK_KEYWORD;
          prev->keyword = KEYWORD_SET;

          goto skip_new_node;
        }

        if (node->ident == string_table[KEYWORD_YOSORO - 1]) {
          prev->type = TOK_KEYWORD;
          prev->keyword = KEYWORD_YOSORO;

          goto skip_new_node;
        }

        if (node->ident == string_table[KEYWORD_VARS - 1]) {
          node->type = TOK_KEYWORD;
          node->keyword = KEYWORD_VARS;
        }

        if (node->ident == string_table[KEYWORD_IHU - 1]) {
          node->type = TOK_KEYWORD;
          node->keyword = KEYWORD_IHU;
        }

        if (node->ident == string_table[KEYWORD_HOR - 1]) {
          node->type = TOK_KEYWORD;
          node->keyword = KEYWORD_HOR;
        }

        if (node->ident == string_table[KEYWORD_WHILE - 1]) {
          node->type = TOK_KEYWORD;
          node->keyword = KEYWORD_WHILE;
        }
      }

      prev = node;
      node = node->next = alloc_token();

    skip_new_node:
      node->type = char_type;

      if (is_char_empty) {
        continue;
      }
    }

    if (node->type == TOK_NUM) {
      node->num = read_int();
    } else {
      switch (char_type) {
      case TOK_IDENT:
        sb_buf[sb_len++] = c;
        break;
      case TOK_LBRACE:
        lbrace_refstack[lbrace_refstack_idx++] = node;
        break;
      case TOK_RBRACE:
        node->brace_ref = lbrace_refstack[--lbrace_refstack_idx];
        node->brace_ref->brace_ref = node;
        break;
      }

      sc(1);
    }
  }

  return root;
}

void token_free(token_t *tok) {
  token_t *next = tok->next;

  free(tok);
  if (next)
    token_free(next);
}

token_t *token_forward(token_t *tok, int n) {
  for (int i = 0; i < n; ++i) {
    if (!tok->next)
      break;
    tok = tok->next;
  }

  return tok;
}

token_t *var_init(token_t *begin_brace) {
  token_t *node = token_forward(begin_brace, 2);

  while (node && node != begin_brace->brace_ref) {
    // if (node->type == TOK_NONE) abort();

    if (node->type == TOK_IDENT) {
      variable_table[variable_table_idx].name = node->ident;
      node = token_forward(node, 2);

      if (node->ident == string_table[7]) {
        variable_table[variable_table_idx].is_array = 1;
        node = token_forward(node, 4);
        variable_table[variable_table_idx].array.base = node->num;
        node = token_forward(node, 2);
        variable_table[variable_table_idx].array.data = (int32_t *)calloc(
            node->num - variable_table[variable_table_idx].array.base + 1,
            sizeof(int32_t));
        node = token_forward(node, 1);
      }

      ++variable_table_idx;
    }

    node = node->next;
  }

  return node;
}

void var_free() {
  for (int i = 0; i < variable_table_idx; ++i) {
    if (variable_table[i].is_array)
      free(variable_table[i].array.data);
  }
}

variable_t *var_get(char *name) {
  for (int i = 0; i < variable_table_idx; ++i) {
    if (variable_table[i].name == name)
      return &variable_table[i];
  }

  return NULL;
}

int match_compare_op(char *op) {
  if (op[0] == 'l') {
    return op[1] == 't' ? CMP_LT : CMP_LE;
  }

  if (op[0] == 'g') {
    return op[1] == 't' ? CMP_GT : CMP_GE;
  }

  return op[0] == 'e' ? CMP_EQ : CMP_NEQ;
}

int compare(int32_t a, int32_t b, int op) {
  // printf("compare: %d %d op=%d\n", a, b, op);

  switch (op) {
  case CMP_LT:
    return a < b;
  case CMP_GT:
    return a > b;
  case CMP_LE:
    return a <= b;
  case CMP_GE:
    return a >= b;
  case CMP_EQ:
    return a == b;
  case CMP_NEQ:
    return a != b;
  }

  abort();
}

int eval(token_t **node) {
  int32_t result = 0;
  int32_t op_add = 1;

  while (*node) {
    switch ((*node)->type) {
    case TOK_NUM: {
      result += (*node)->num * op_add;
      op_add = 0;
    } break;
    case TOK_IDENT: {
      variable_t *var = var_get((*node)->ident);
      if (var->is_array) {
        *node = token_forward(*node, 2);
        result += var->array.data[eval(node) - var->array.base] * op_add;
      } else {
        result += var->num * op_add;
      }
    } break;
    case TOK_ADD:
      op_add = 1;
      break;
    case TOK_SUB:
      op_add = -1;
      break;
    default:
      return result;
    }

    if ((*node)->type == TOK_NUM || (*node)->type == TOK_IDENT) {
      op_add = 0;
    }

    (*node) = (*node)->next;
  }
}

token_t tokenblock_terminator = {0};
token_t *block_get_end_token(token_t *lbrace, token_t *curr) {
  token_t *node = curr;
  token_t *prev = NULL;

  while (node) {
    if (node == lbrace->brace_ref) {
      return prev;
    }

    prev = node;
    node = node->next;
  }

  abort();
}

void run_tokens(token_t *tok) {
  token_t *node = tok;
  token_t *prev = tok;

  while (node) {
    if (node->type == TOK_NONE)
      break;

    if (node->type == TOK_KEYWORD) {
      if (node->keyword == KEYWORD_VARS) {
        node = var_init(prev);
      }

      if (node->keyword == KEYWORD_SET) {
        node = token_forward(node, 1);
        variable_t *var = var_get(node->ident);
        node = token_forward(node, 2);
        int32_t val = eval(&node);

        if (!var)
          abort();

        if (var->is_array) {
          node = token_forward(node, 2);

          var->array.data[val - var->array.base] = eval(&node);
        } else {
          var->num = val;
        }
        continue;
      }

      if (node->keyword == KEYWORD_YOSORO) {
        node = token_forward(node, 1);
        print_int(eval(&node));
        continue;
      }

      if (node->keyword == KEYWORD_IHU) {
        token_t *lbrace = prev;
        node = token_forward(node, 1);
        int cmp = match_compare_op(node->ident);
        node = token_forward(node, 2);

        int v1 = eval(&node);
        node = token_forward(node, 1);
        int v2 = eval(&node);

        if (!compare(v1, v2, cmp)) {
          node = lbrace->brace_ref;
        }
        continue;
      }

      if (node->keyword == KEYWORD_HOR) {
        token_t *lbrace = prev;
        node = token_forward(node, 1);
        variable_t *var = var_get(node->ident);
        node = token_forward(node, 2);
        var->num = eval(&node);
        node = token_forward(node, 1);
        int endval_int = eval(&node);

        if (node->type == TOK_RBRACE)
          continue;

        token_t *end_token = block_get_end_token(lbrace, node);
        end_token->next = &tokenblock_terminator;
        for (; var->num <= endval_int; ++var->num) {
          run_tokens(node);
        }
        end_token->next = lbrace->brace_ref;

        prev = lbrace->brace_ref;
        node = prev->next;
        continue;
      }

      if (node->keyword == KEYWORD_WHILE) {
        token_t *lbrace = prev;
        node = token_forward(node, 1);
        int cmp = match_compare_op(node->ident);
        node = token_forward(node, 2);
        token_t *e1 = node;
        token_t *e1_tmp = e1;
        node = token_forward(node, 2);
        token_t *e2 = node;
        token_t *e2_tmp = e2;

        token_t *end_token = block_get_end_token(lbrace, node);
        end_token->next = &tokenblock_terminator;
        while (compare(eval(&e1_tmp), eval(&e2_tmp), cmp)) {
          run_tokens(node);
          e1_tmp = e1;
          e2_tmp = e2;
        }
        end_token->next = lbrace->brace_ref;

        prev = lbrace->brace_ref;
        node = prev->next;
        continue;
      }
    }

    prev = node;
    node = node->next;
  }
}

int main() {
  fill_inbuf();
  token_t *tok = tokenize();

  run_tokens(tok);
  flush_output();
  write(1, "\n", 1);

  var_free();

  _exit(0);
}