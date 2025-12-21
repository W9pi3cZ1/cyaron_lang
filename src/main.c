#ifndef NO_CUSTOM_INC
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#endif

#ifndef NO_STD_INC
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#endif

void read_src(char **src) {
  FILE *input = stdin;
  size_t capacity = 32;
  *src = malloc(capacity);
  size_t input_size = 0;
  while (!feof(input)) {
    if (input_size >= capacity) {
      capacity *= 2;
      *src = realloc(*src, capacity);
    }
    (*src)[input_size] = fgetc(input);
    input_size++;
  }
  (*src)[input_size - 1] = '\0'; // Replace EOF
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
  clock_t start_time;
  clock_t end_time;
  size_t time_spent;
  char *src;
  CLOCK_FUNC(start_time, end_time, time_spent, read_src, &src);
  Lexer *lexer = lexer_create(src);
  // lexer_tokenize(lexer);
  CLOCK_FUNC(start_time, end_time, time_spent, lexer_tokenize, lexer);
#ifndef NO_DEBUG
  debug_lexer(lexer);
#endif
  Parser *parser = parser_create(&lexer->toks);
  // parser_parse(parser);
  CLOCK_FUNC(start_time, end_time, time_spent, parser_parse, parser);
#ifndef NO_DEBUG
  debug_parser(parser);
#endif
  Interpreter *interpreter =
      interpreter_create(&parser->stmts, &parser->var_decls);
  // interpreter_execute(interpreter);
  CLOCK_FUNC(start_time, end_time, time_spent, interpreter_execute,
             interpreter);
#ifndef NO_DEBUG
  interpreter_stats(interpreter);
#endif
  interpreter_free(interpreter);
  parser_free(parser);
  lexer_free(lexer);
  free(src);
}

int main() {
  //   FILE *log = fopen("log.txt", "w");
  //   set_glob_log_output(log);
  // printf("CYaRon!!\n");
  clock_t start_time;
  clock_t end_time;
  size_t time_spent;
  CLOCK_FUNC(start_time, end_time, time_spent, test);
  //   fclose(log);
  return 0;
}
