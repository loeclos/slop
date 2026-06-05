#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char *readline(char *prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char *cpy = malloc(strlen(buffer) + 1);
  strcopy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

void add_history(char *unused) {}

#else
#include <editline/readline.h>
#include <histedit.h>
#endif

int main(int argc, char **argv) {
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Slop = mpc_new("slop");

  mpca_lang(MPCA_LANG_DEFAULT, "                                              \
        number   : /-?[0-9]+/ ;                            \
        operator : '+' | '-' | '*' | '/' ;                 \
        expr     : <number> | '(' <operator> <expr>+ ')' ; \
        slop     : /^/ <operator> <expr>+ /$/ ;            \
",
            Number, Operator, Expr, Slop);

  puts("Slop version 0.0.0.1");
  puts("Be ready for the future.\n");
  puts("Press CTRL+C to Exit\n");

  while (1) {
    char *input = readline("slop> ");

    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Slop, &r)) {
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Slop);

  return 0;
}
