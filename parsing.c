#include "mpc.h"
#include <stdbool.h>

#ifdef _WIN32

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

enum { LVAL_NUM, LVAL_FLOAT, LVAL_ERR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

union Num {
  long i;
  double f;
};

typedef struct {
  int type;
  union Num num;
  int err;
  bool isFloat;
} lval;

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num.i = x;
  v.isFloat = false;
  return v;
}

lval lval_float(double x) {
  lval v;
  v.type = LVAL_NUM;
  v.num.f = x;
  v.isFloat = true;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
  case LVAL_NUM:
    printf("%li", v.num);
    break;

  case LVAL_ERR:
    if (v.err == LERR_DIV_ZERO) {
      printf("Error: Division by Zero!");
    }
  }
}

lval eval_op(lval x, char *op, lval y) {

  if (x.type == LVAL_ERR) {
    return x;
  }
  if (y.type == LVAL_ERR) {
    return y;
  }

  if (strcmp(op, "+") == 0) {
    if (x.isFloat && !y.isFloat) {
      return lval_float(x.num.f + y.num.i);
    }
    if (y.isFloat && !x.isFloat) {
      return lval_float(x.num.i + y.num.f);
    }
    if (x.isFloat && y.isFloat) {
      return lval_float(x.num.f + y.num.f);
    }
    if (!x.isFloat && !y.isFloat) {
      return lval_num(x.num.i + y.num.i);
    }
  }
  if (strcmp(op, "-") == 0) {
    if (x.isFloat && !y.isFloat) {
      return lval_float(x.num.f - y.num.i);
    }
    if (y.isFloat && !x.isFloat) {
      return lval_float(x.num.i - y.num.f);
    }
    if (x.isFloat && y.isFloat) {
      return lval_float(x.num.f - y.num.f);
    }
    if (!x.isFloat && !y.isFloat) {
      return lval_num(x.num.i - y.num.i);
    }
  }

  if (strcmp(op, "/") == 0) {

    if (y.num.i == 0 && !y.isFloat || y.num.f == 0 && y.isFloat) {
      return lval_err(LERR_DIV_ZERO);
    }

    if (x.isFloat && !y.isFloat) {
      return lval_float(x.num.f / y.num.i);
    }
    if (y.isFloat && !x.isFloat) {
      return lval_float(x.num.i / y.num.f);
    }
    if (x.isFloat && y.isFloat) {
      return lval_float(x.num.f / y.num.f);
    }
    if (!x.isFloat && !y.isFloat) {
      return lval_num(x.num.i / y.num.i);
    }
  }

  if (strcmp(op, "*") == 0) {
    if (x.isFloat && !y.isFloat) {
      return lval_float(x.num.f * y.num.i);
    }
    if (y.isFloat && !x.isFloat) {
      return lval_float(x.num.i * y.num.f);
    }
    if (x.isFloat && y.isFloat) {
      return lval_float(x.num.f * y.num.f);
    }
    if (!x.isFloat && !y.isFloat) {
      return lval_num(x.num.i * y.num.i);
    }
  }

  if (strcmp(op, "%") == 0) {
    if (x.isFloat && !y.isFloat) {
      return lval_float(lroundf(x.num.f) % y.num.i);
    }
    if (y.isFloat && !x.isFloat) {
      return lval_float(x.num.i % lroundf(y.num.f));
    }
    if (x.isFloat && y.isFloat) {
      return lval_float(lroundf(x.num.f) % lroundf(y.num.f));
    }
    if (!x.isFloat && !y.isFloat) {
      return lval_num(x.num.i % y.num.i);
    }
  }

  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t *t) {

  if (strstr(t->tag, "integer")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);

    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  if (strstr(t->tag, "float")) {
    errno = 0;
    double x = strtof(t->contents, NULL);

    return errno != ERANGE ? lval_float(x) : lval_err(LERR_BAD_NUM);
  }

  char *op = t->children[1]->contents;

  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
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

  mpca_lang(
      MPCA_LANG_DEFAULT,
      "                                                                            \
        integer  : /-?[0-9]+/ ;                                                                             \
        float    : /[+-]?(([0-9]+[.]+[0-9]*)|([.][0-9]+))/ ;                                                \
        operator : '+' | '-' | '*' | '/' | '%' ;                                                            \
        expr     : <float> | <integer> | '('  <operator> <expr>+ ')'  ; \
        slop     : /^/ <operator> <expr>+ /$/ ;                                                     \
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
      printf("Pondering input...\n");

      lval result = eval(r.output);

      if (!result.isFloat) {
        printf("The logic returneth: %li\n", result.num.i);
      }
      if (result.isFloat) {
        printf("The logic returneth: %li\n", result.num.i);
      }

      // mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Integer, Float, Operator, Expr, Slop);

  return 0;
}
