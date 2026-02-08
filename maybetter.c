/* Code from [Luogu R256198099](https://www.luogu.com.cn/record/256198099) */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#define INBUF_SIZE (1 << 12)
#define OUTBUF_SIZE (1 << 5)
#define TOKEN_POOL_SIZE (1 << 12)

typedef struct {
  char *name;
  uint8_t is_array;
  union {
    int32_t num;
    struct {
      int32_t base;
      int32_t *data;
    } array;
  };
} variable_t;

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
#define TOK_EXT_SKIP (13)
#define TOK_EXT_VAROFF (14)

#define KEYWORD_NONE (0)
#define KEYWORD_SET (1)
#define KEYWORD_YOSORO (2)
#define KEYWORD_VARS (3)
#define KEYWORD_IHU (4)
#define KEYWORD_HOR (5)
#define KEYWORD_WHILE (6)

#define ANALYZE_STAGE_NONE (0)
#define ANALYZE_STAGE_DONE (1)
#define ANALYZE_STAGE_SKIP (2)
typedef struct __node {
  // struct __node *next;
  uint16_t analyzer_data;
  uint8_t analyze_stage;
  uint8_t type;
  union {
    uint8_t keyword;
    char *ident;
    struct {
      int32_t num;
      int32_t varoff_attr; // 正负号代表var取值的正负
                           // 绝对值表示跳过的token数(因为正常set肯定比varoff长)
    };
    struct __node *brace_ref;
  };
  variable_t *var_ptr;
} token_t;

#define CMP_EQ_BIT (1 << 0)
#define CMP_GT_BIT (1 << 1)
#define CMP_WITHSZCMP_BIT (1 << 2)
#define CMP_LT (CMP_WITHSZCMP_BIT)
#define CMP_GT (CMP_WITHSZCMP_BIT | CMP_GT_BIT)
#define CMP_LE (CMP_WITHSZCMP_BIT | CMP_EQ_BIT)
#define CMP_GE (CMP_WITHSZCMP_BIT | CMP_EQ_BIT | CMP_GT_BIT)
#define CMP_EQ (CMP_EQ_BIT)
#define CMP_NEQ (0)

typedef char stringtable_entry_t[16];

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

void dump_tokens(token_t *tok);

static inline void fill_inbuf() {
  inbuf_remain = read(0, inbuf, INBUF_SIZE);
  inptr = inbuf;
}

static inline char gc() {
  if (inbuf_remain <= 0)
    fill_inbuf();
  return (inbuf_remain <= 0) ? -1 : *inptr++;
}

static inline char pc() {
  if (inbuf_remain <= 0)
    fill_inbuf();
  return (inbuf_remain <= 0) ? -1 : *inptr;
}

static inline void sc(int n) {
  while (n > inbuf_remain) {
    n -= inbuf_remain;
    fill_inbuf();
  }

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
  if (outbuf_pos > OUTBUF_SIZE - 12)
    flush_output();

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

static inline token_t *alloc_token() {
  // if (token_pool_idx >= TOKEN_POOL_SIZE) abort();
  return &token_pool[token_pool_idx++];
}

uint8_t char_type_table[256] = {
    ['{'] = TOK_LBRACE,   ['}'] = TOK_RBRACE, ['['] = TOK_LBRACKET,
    [']'] = TOK_RBRACKET, [','] = TOK_COMMA,  [':'] = TOK_COLON,
    ['+'] = TOK_ADD,      ['-'] = TOK_SUB,    ['.'] = TOK_2DOT,
    ['0'] = TOK_NUM,      ['1'] = TOK_NUM,    ['2'] = TOK_NUM,
    ['3'] = TOK_NUM,      ['4'] = TOK_NUM,    ['5'] = TOK_NUM,
    ['6'] = TOK_NUM,      ['7'] = TOK_NUM,    ['8'] = TOK_NUM,
    ['9'] = TOK_NUM,      ['a'] = TOK_IDENT,  ['b'] = TOK_IDENT,
    ['c'] = TOK_IDENT,    ['d'] = TOK_IDENT,  ['e'] = TOK_IDENT,
    ['f'] = TOK_IDENT,    ['g'] = TOK_IDENT,  ['h'] = TOK_IDENT,
    ['i'] = TOK_IDENT,    ['j'] = TOK_IDENT,  ['k'] = TOK_IDENT,
    ['l'] = TOK_IDENT,    ['m'] = TOK_IDENT,  ['n'] = TOK_IDENT,
    ['o'] = TOK_IDENT,    ['p'] = TOK_IDENT,  ['q'] = TOK_IDENT,
    ['r'] = TOK_IDENT,    ['s'] = TOK_IDENT,  ['t'] = TOK_IDENT,
    ['u'] = TOK_IDENT,    ['v'] = TOK_IDENT,  ['w'] = TOK_IDENT,
    ['x'] = TOK_IDENT,    ['y'] = TOK_IDENT,  ['z'] = TOK_IDENT,
};

#define classify_char(c) char_type_table[(uint8_t)(c)]

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

        int str_offset = (stringtable_entry_t *)node->ident - string_table;

        if (str_offset < KEYWORD_WHILE) {
          if (str_offset < KEYWORD_YOSORO) {
            prev->type = TOK_KEYWORD;
            prev->keyword = str_offset + 1;
            goto skip_new_node;
          } else {
            node->type = TOK_KEYWORD;
            node->keyword = str_offset + 1;
          }
        }
      }

      prev = node;
      node = alloc_token();

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

#define token_free(tok)
#define token_forward(tok, n) (tok + n)

// static __always_inline token_t *token_forward(token_t *tok, int n) {
//   // for (int i = 0; i < n; ++i) {
//   //   if (!tok->next) break;
//   //   tok = tok->next;
//   // }

//   return tok + n;
// }

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

    node = token_forward(node, 1);
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

char match_compare_op(char ch0, char ch1) {
  static const int8_t lut[25] = {
      ['l' ^ 't'] = 1, ['e' ^ 'q'] = 2, ['l' ^ 'e'] = 3,
      ['g' ^ 't'] = 4, ['n' ^ 'e'] = 5, ['g' ^ 'e'] = 6,
  };
  char r = lut[ch0 ^ ch1];
  return r;
}

char compare(int32_t a, int32_t b, char op) {
  // lt=1 eq=2 le=3 gt=4 neq=5 ge=6
  return (op >> ((a > b) + (a >= b))) & 1;
}

// 专门处理数组索引的eval
static inline int32_t eval_index(token_t **node) {
  int32_t result = 0;
  int32_t sign = 1;

  while (1) {
    token_t *tok = *node;
    switch (tok->type) {
    case TOK_NUM:
      result += sign * tok->num;
      sign = 1;
      break;
    case TOK_IDENT: {
      variable_t *var = tok->var_ptr;
      result += sign * var->num;
      sign = 1;
      break;
    }
    case TOK_ADD:
      sign *= 1;
      break;
    case TOK_SUB:
      sign *= -1;
      break;
    default:
      return result;
    }
    (*node)++;
  }
}

// 主eval函数 会在返回是向var_ref写入此eval块唯一的变量引用
// 如果不止一个就写入NULL
int eval(token_t **node, token_t **var_ref) {
  int32_t result = 0;
  int32_t sign = 1;

  token_t *ref = NULL;
  int var_num = 0;

  while (*node) {
    switch ((*node)->type) {
    case TOK_NUM: {
      result += (*node)->num * sign;
      sign = 1;
      break;
    }
    case TOK_IDENT: {
      ref = *node;
      variable_t *var = ref->var_ptr;
      if (var->is_array) {
        *node = token_forward(ref, 2);
        int32_t idx = eval_index(node);
        result += var->array.data[idx - var->array.base] * sign;
      } else {
        result += var->num * sign;
      }
      sign = 1;
      break;
    }
    case TOK_ADD:
      sign *= 1;
      break;
    case TOK_SUB:
      sign *= -1;
      break;
    default:
      if (var_ref)
        *var_ref = var_num == 1 ? ref : NULL;
      return result;
    }

    *node = token_forward(*node, 1);
  }

  return result;
}

void optimize_hor_set(token_t *lbrace, token_t *exec_begin) {}

void optimize_while_set(token_t *lbrace, token_t *exec_begin) {}

void token_bind_vars(token_t *tok) {
  for (int i = 0; i < token_pool_idx; i++) {
    token_t *tok = &token_pool[i];
    if (tok->type == TOK_IDENT) {
      tok->var_ptr = var_get(tok->ident);
    }
  }
}

void run_tokens(token_t *tok) {
  token_t *node = tok;
  token_t *prev = tok;

  uint8_t yosoro_count = 0;

  while (node) {
    // printf("ip=%ld\r", node - tok);
    // dump_tokens(node);
    // asm("int3");
    if (node->type == TOK_EXT_VAROFF) {
      int val = node->var_ptr->num;
      int attr = node->varoff_attr;

      if (attr < 0) {
        val = -val;
        attr = -attr;
      }

      node->num = val + node->num;

      node = token_forward(node, attr);
      prev = node;
      continue;
    }

    if (node->type == TOK_RBRACE) {
      //  如果有打印 或已经被优化就跳过优化步骤
      if (node->analyze_stage != ANALYZE_STAGE_NONE)
        return;

      if (yosoro_count) {
        node->analyze_stage = ANALYZE_STAGE_SKIP;
        return;
      }

      token_t *lbrace = node->brace_ref;
      token_t *kw = token_forward(lbrace, 1);

      if (kw->keyword == KEYWORD_HOR) {
        optimize_hor_set(lbrace, tok);
      } else if (kw->keyword == KEYWORD_WHILE) {
        optimize_while_set(lbrace, tok);
      }

      node->analyze_stage = ANALYZE_STAGE_DONE;
      return;
    }

    if (node->type == TOK_NONE)
      break;

    if (node->type == TOK_KEYWORD) {
      if (node->keyword == KEYWORD_VARS) {
        node = var_init(prev);

        token_bind_vars(node);
        // dump_tokens(tok);
      }

      if (node->keyword == KEYWORD_SET) {
        token_t *kw = node;
        token_t *first_var = NULL;
        node = token_forward(node, 1);
        variable_t *var = node->var_ptr;
        node = token_forward(node, 2);
        int32_t val = eval(&node, &first_var);

        if (var->is_array) {
          node = token_forward(node, 2);

          var->array.data[val - var->array.base] = eval(&node, NULL);
        } else {
          var->num = val;

          if (first_var && first_var->var_ptr == var) {
            kw->var_ptr = var;

            token_t *pre_var = token_forward(kw, -1);
            int32_t attr =
                (pre_var->type != TOK_ADD && pre_var->type != TOK_COMMA) ? -1
                                                                         : 1;
            attr += node - kw;
            kw->varoff_attr = attr;

            int val = var->num;
            var->num = 0;
            kw->num = eval(&node, NULL);
            var->num = val;
          }
        }

        prev = node;
        continue;

        // printf("after set: %s\n", var->name);
        // dump_tokens(node);
      }

      if (node->keyword == KEYWORD_YOSORO) {
        if (yosoro_count == 0xff)
          abort();
        ++yosoro_count;
        node = token_forward(node, 1);
        print_int(eval(&node, NULL));
        prev = node;
        continue;
      }

      if (node->keyword == KEYWORD_IHU) {
        // print_int(123);
        token_t *lbrace = prev;
        node = token_forward(node, 1);
        int cmp = match_compare_op(node->ident[0], node->ident[1]);
        node = token_forward(node, 2);

        int v1 = eval(&node, NULL);
        node = token_forward(node, 1);
        int v2 = eval(&node, NULL);

        if (compare(v1, v2, cmp)) {
          run_tokens(node);
        }

        node = lbrace->brace_ref;
      }

      if (node->keyword == KEYWORD_HOR) {
        // print_int(456);
        token_t *lbrace = prev;
        node = token_forward(node, 1);
        variable_t *var = node->var_ptr;
        int *var_field = &var->num;
        node = token_forward(node, 2);
        if (var->is_array) {
          var_field = &var->array.data[eval_index(&node) - var->array.base];
          node = token_forward(node, 2);
        }
        *var_field = eval(&node, NULL);
        node = token_forward(node, 1);
        int endval_int = eval(&node, NULL);

        // if (node->type == TOK_RBRACE) continue;

        for (; *var_field <= endval_int; ++*var_field) {
          run_tokens(node);

          if (*var_field == endval_int)
            break;
        }

        node = lbrace->brace_ref;
      }

      if (node->keyword == KEYWORD_WHILE) {
        // print_int(789);
        token_t *lbrace = prev;
        node = token_forward(node, 1);
        int cmp = match_compare_op(node->ident[0], node->ident[1]);
        node = token_forward(node, 2);
        token_t *e1 = node;
        token_t *e1_tmp = e1;
        eval(&node, NULL);
        node = token_forward(node, 1);
        token_t *e2 = node;
        token_t *e2_tmp = e2;
        eval(&node, NULL);

        // dump_tokens(node);

        while (compare(eval(&e1_tmp, NULL), eval(&e2_tmp, NULL), cmp)) {
          run_tokens(node);
          e1_tmp = e1;
          e2_tmp = e2;
        }

        node = lbrace->brace_ref;
      }
    }

    prev = node;
    node = token_forward(node, 1);
  }
}

const char *token_type_strs[] = {
    "NONE", "LBRACE",  "RBRACE", "LBRACKET", "RBRACKET", "COMMA", "COLON",
    "2DOT", "KEYWORD", "IDENT",  "NUM",      "ADD",      "SUB",
};

const char *token_keyword_strs[] = {
    "NONE", "SET", "YOSORO", "VARS", "IHU", "HOR", "WHILE",
};

void dump_tokens(token_t *tok) {
  token_t *node = tok;
  int i = 0;

  printf("#####dumping tokens#####\n");

  while (node && node->type != TOK_NONE) {
    printf("% 4d: %s", i, token_type_strs[node->type]);

    if (node->type == TOK_KEYWORD) {
      printf("=> %s", token_keyword_strs[node->keyword]);
    }

    if (node->type == TOK_IDENT) {
      printf("=> %s", node->ident);
      if (node->var_ptr) {
        printf("(var, vaild=%d)", node->ident == node->var_ptr->name);
      }
    }

    if (node->type == TOK_NUM) {
      printf("=> %d", node->num);
    }

    printf("\n");

    node = token_forward(node, 1);
    ++i;
  }

  printf("#####dump finished#####\n");
  fflush(stdout);
}

int main() {
  // const char *cmp_strs[] = {
  //   "le",
  //   "lt",
  //   "eq",
  //   "neq",
  //   "ge",
  //   "gt",
  // };

  // struct {
  //   int i1, i2;
  // } testcases[] = {
  //   {0, 1},
  //   {1, 0},
  //   {0, 0},
  //   {1, 1},
  //   {2, 1},
  //   {1, 2},
  //   {23, 45},
  //   {45, 23},
  // };

  // for (int i = 0; i < 6; ++i) {
  //   int op = match_compare_op(cmp_strs[i]);
  //   printf("%s=> %x\n", cmp_strs[i], op);

  //   for (int j = 0; j < 8; ++j) {
  //     printf("  %d %d %s => %d\n", testcases[j].i1, testcases[j].i2,
  //            cmp_strs[i], compare(testcases[j].i1, testcases[j].i2, op));
  //   }
  // }

  fill_inbuf();
  token_t *tok = tokenize();

  run_tokens(tok);
  flush_output();
  write(1, "\n", 1);

  var_free();

  _exit(0);
  return 0;
}