#ifndef NO_CUSTOM_INC
#include "lexer.h"
#include "parser.h"
#endif

#ifdef CODEGEN
#ifndef NO_CUSTOM_INC
#include "codegen.h"
#include "vm.h"
#endif
#else
#ifndef NO_CUSTOM_INC
#include "interpreter.h"
#endif
#endif

#ifndef NO_STD_INC
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#endif

// void read_src(char **src) {
//   FILE *input = stdin;
//   size_t capacity = 32;
//   *src = malloc(capacity);
//   size_t input_size = 0;
//   while (!feof(input)) {
//     if (input_size >= capacity) {
//       capacity *= 2;
//       *src = realloc(*src, capacity);
//     }
//     (*src)[input_size] = fgetc(input);
//     input_size++;
//   }
//   (*src)[input_size - 1] = '\0'; // Replace EOF
// }

void read_src(char **src) {
  FILE *input = stdin;
  size_t capacity = 256;
  *src = malloc(capacity);
  // if (*src == NULL) return;
  size_t total_read = 0;
  while (1) {
    size_t remaining = capacity - total_read - 1;
    if (remaining == 0) {
      capacity *= 2;
      *src = realloc(*src, capacity);
      // if (*src == NULL) return;
      remaining = capacity - total_read - 1;
    }
    size_t nread = fread(*src + total_read, 1, remaining, input);
    total_read += nread;
    if (nread < remaining) {
      break;
    }
  }
  (*src)[total_read] = '\0';
}

#ifndef NO_CLOCK
#define CLOCK_FUNC(start, end, time_spent, func, ...)                          \
  start = clock();                                                             \
  func(__VA_ARGS__);                                                           \
  end = clock();                                                               \
  time_spent = (end - start) * 1000000 / CLOCKS_PER_SEC;                       \
  printf("\n%s() completed in %zu us.\n", #func, time_spent)
#else
#define CLOCK_FUNC(start, end, time_spent, func, ...) func(__VA_ARGS__);
#endif

void test() {

#ifndef NO_CLOCK
  clock_t start_time;
  clock_t end_time;
  size_t time_spent;
#endif

  char *src;
  CLOCK_FUNC(start_time, end_time, time_spent, read_src, &src);
  Lexer lexer;
  lexer_init(&lexer, src);
  // lexer_tokenize(lexer);
  CLOCK_FUNC(start_time, end_time, time_spent, lexer_tokenize, &lexer);
#ifndef NO_DEBUG
  debug_lexer(&lexer);
#endif
  Parser parser;
  parser_init(&parser, &lexer.toks);
  // parser_parse(parser);
  CLOCK_FUNC(start_time, end_time, time_spent, parser_parse, &parser);
#ifndef NO_DEBUG
  debug_parser(&parser);
#endif

#ifdef CODEGEN

  CodeGen cg;
  cg_init(&cg, &parser.stmts, &parser.var_decls);
  CLOCK_FUNC(start_time, end_time, time_spent, cg_gen, &cg);
#ifndef NO_DEBUG
  cg_debug(&cg);
#endif
  CyrVM cyr_vm;
  cyr_vm_init(&cyr_vm, &parser.var_decls, &cg.codes);
  CLOCK_FUNC(start_time, end_time, time_spent, cyr_vm_execute, &cyr_vm);
  cg_free(&cg);

#else

  Interpreter interpreter;
  interpreter_init(&interpreter, &parser.stmts, &parser.var_decls);
  // interpreter_execute(interpreter);
  CLOCK_FUNC(start_time, end_time, time_spent, interpreter_execute,
             &interpreter);
#ifndef NO_DEBUG
  interpreter_stats(&interpreter);
#endif
  interpreter_free(&interpreter);

#endif
  parser_free(&parser);
  lexer_free(&lexer);
  free(src);
}

int main() {

#ifndef NO_CLOCK
  clock_t start_time;
  clock_t end_time;
  size_t time_spent;
#endif

  //   FILE *log = fopen("log.txt", "w");
  //   set_glob_log_output(log);
  // printf("CYaRon!!\n");

  CLOCK_FUNC(start_time, end_time, time_spent, test);
  //   fclose(log);
  return 0;
}
