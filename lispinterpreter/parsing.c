#include<stdio.h>
#include<stdlib.h>
#include <math.h>
#include"mpc.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  if (strcmp(op, "%") == 0) { return x % y; }
  if (strcmp(op, "add") == 0) { return x + y; }
  if (strcmp(op, "sub") == 0) { return x - y; }
  if (strcmp(op, "mul") == 0) { return x * y; }
  if (strcmp(op, "div") == 0) { return x / y; }
  if (strcmp(op, "^") == 0) { return pow(x,y); }
  if (strcmp(op, "min") == 0) { return min(x,y); }
  if (strcmp(op, "max") == 0) { return max(x,y); }

  return 0;
}

long eval(mpc_ast_t* t) {
  
  /* If tagged as number return it directly. */ 
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }
  
  /* The operator is always second child. */
  char* op = t->children[1]->contents;
  
  /* We store the third child in `x` */
  long x = eval(t->children[2]);
  
  /* Iterate the remaining children and combining. */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }
  
  return x;  
}



int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expression = mpc_new("expr");
  mpc_parser_t* Mylisp = mpc_new("mylisp");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                           \
    number : /-?[0-9]+/; \
    operator    : '+' | '-' | '*' | '/' | '%' | /add/ | /mul/ | /sub/ | /div/ | '^' | /min/ | /max/;           \
    expr      : <number> | '('' <operator> <expr>+ ')'; \
    mylisp     : /^/ <operator> <expr>+ /$/ ;                    \
  ",
  Number, Operator, Expression ,Mylisp);

  puts("myLisp Version 0.0.0.0.3");
  puts("Press Ctrl+c to Exit\n");

  while (1) {

  	char* input = readline("mylisp> ");

    add_history(input);

    mpc_result_t r;

	if (mpc_parse("<stdin>", input, Mylisp, &r)) {
	  /* On Success Print the AST */
	  long result = eval(r.output);
      printf("%li\n", result);
	  mpc_ast_delete(r.output);
	} else {
	  /* Otherwise Print the Error */
	  mpc_err_print(r.error);
	  mpc_err_delete(r.error);
	}

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expression, Mylisp);
  return 0;
}