#include "mpc.h"
#include <stdbool.h>
#include <stdio.h>

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

/* Create Enumeration of Possible Error Types */
enum { LMIS_DIV_ZERO, LMIS_BAD_OP, LMIS_BAD_NUM };

/* Create Enumeration of Possible lval Types */
enum { LVAL_INT, LVAL_MISTAKE, LVAL_SYM, LVAL_SFORM };

typedef struct lval {
  int type;
  long num;

  char *err;
  char *sym;

  int count;
  struct lval **cell;
} lval;

/* Create a new number type lval */
lval *lval_int(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_INT;
  v->num = x;
  return v;
}

/* Create a new error type lval */
lval *lval_mistake(char *m) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_INT;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

lval *lval_sym(char *s) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval *lval_sexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SFORM;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_INT:
    break;

  case LVAL_MISTAKE:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;

  case LVAL_SFORM:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }

    free(v->cell);
    break;
  }

  free(v);
}

lval *lval_add(lval *v, lval *x) {

  v->count++;
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  v->cell[v->count - 1] = x;

  return v;
}

lval *lval_read_int(mpc_ast_t *t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_int(x) : lval_mistake("invalid integer");
}

lval *lval_read(mpc_ast_t *t) {
  if (strstr(t->tag, "number")) {
    return lval_read_int(t);
  }
  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  lval *x = NULL;
  if (strcmp(t->tag, ">") == 0) {
    x = lval_sexpr();
  }
  if (strstr(t->tag, "sexpr")) {
    x = lval_sexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, ")") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

void lval_expr_print(lval *v, char open, char close) {

  putchar(open);

  for (int i = 0; i < v->count; i++) {

    lval_output(v->cell[i]);

    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }

  putchar(close);
}

void lval_output(lval *v) {
  switch (v->type) {
  case LVAL_INT:
    printf("%li", v->num);
    break;

  case LVAL_MISTAKE:
    printf("Error: %s", v->err);
    break;

  case LVAL_SYM:
    printf("%s", v->sym);
    break;
  case LVAL_SFORM:
    lval_expr_print(v, '(', ')');
    break;
  }
}

lval eval_op(lval x, char *op, lval y) {

  /* If either value is an error return it */
  if (x.type == LVAL_MISTAKE) {
    return x;
  }
  if (y.type == LVAL_MISTAKE) {
    return y;
  }

  /* Otherwise do maths on the number values */
  if (strcmp(op, "+") == 0) {
    return lval_int(x.num + y.num);
  }
  if (strcmp(op, "-") == 0) {
    return lval_int(x.num - y.num);
  }
  if (strcmp(op, "*") == 0) {
    return lval_int(x.num * y.num);
  }
  if (strcmp(op, "/") == 0) {
    /* If second operand is zero return error */
    return y.num == 0 ? lval_mistake(LMIS_DIV_ZERO) : lval_int(x.num / y.num);
  }

  return lval_mistake(LMIS_BAD_OP);
}

lval eval(mpc_ast_t *t) {

  if (strstr(t->tag, "int")) {
    /* Check if there is some error in conversion */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_int(x) : lval_mistake(LMIS_BAD_NUM);
  }

  char *op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "form")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char **argv) {

  mpc_parser_t *Integer = mpc_new("int");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Formulation = mpc_new("form");
  mpc_parser_t *SFormulation = mpc_new("sform");
  mpc_parser_t *Slop = mpc_new("slop");

  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                     \
      int   : /-?[0-9]+/ ;                             \
      symbol : '+' | '-' | '*' | '/' ;                  \
      sform : '(' <form>* ')' ; \
      form     : <int> | <symbol> | <sform> ;  \
      slop    : /^/ <form>+ /$/ ;             \
    ",
            Integer, Symbol, SFormulation, Formulation, Slop);

  puts("Slop Version 0.0.0.1 - Welcome to the future.");
  puts("Press Ctrl+c to Exit\n");

  while (1) {

    char *input = readline("| slop > ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Slop, &r)) {
      lval output = eval(r.output);
      lval_outputln(output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Integer, Symbol, SFormulation, Formulation, Slop);

  return 0;
}
