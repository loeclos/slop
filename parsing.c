#include "mpc.h"
#include <time.h>
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

int eval_ops(int x, char* op, int y) {
  if ()
}

int eval(mpc_ast_t* t) {

  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  char* op = t->children[1]->contents;

  long x = eval(t->children[2]);

  int i = 0;
  while (strstr(t->children[i], "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;

}

int main(int argc, char **argv) {
  mpc_parser_t *Integer = mpc_new("integer");
  mpc_parser_t *Float = mpc_new("float");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Slop = mpc_new("slop");

  mpca_lang(MPCA_LANG_DEFAULT, "                           \
        integer  : /-?[0-9]+/ ;                            \
        float    : /[+-]?(([0-9]+[.]+[0-9]*)|([.][0-9]+))/ ;                 \
        operator : '+' | '-' | '*' | '/' ;                 \
        expr     : <float> | <integer> | '(' <operator> <expr>+ ')' ; \
        slop     : /^/ <operator> <expr>+ /$/ ;            \
",
Integer, Float, Operator, Expr, Slop);

  puts("Slop version 0.0.0.1");
  puts("Be ready for the future.\n");
  puts("Press CTRL+C to Exit\n");

  while (1) {
    char *input = readline("slop> ");

    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Slop, &r)) {
      mpc_ast_t* a = r.output;

      printf("Pondering input...\n");
      clock_t begin = clock();

      int result = eval(a);

      clock_t end = clock();
      double time_spent = (double)(end - begin) / CLOCKS_PER_SECOND;

      printf("The logic returneth: %l\n", result);
      printf("In %.2lf seconds.");
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Integer, Float, Operator, Expr, Slop);

  return 0;
}
