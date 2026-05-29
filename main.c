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
  puts("Slop version 0.0.0.1");
  puts("Be ready for the future.\n");
  puts("Press CTRL+C to Exit\n");

  while (1) {
    char *input = readline("slop> ");

    add_history(input);

    printf("No you're a %s\n", input);

    free(input);
  }

  return 0;
}
