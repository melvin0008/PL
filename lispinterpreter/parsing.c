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

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR};


typedef struct lval{
  int type;
  long num;

  char* err;
  char* sym;

  int count;
  struct lval** cell;
} lval;

void lval_print(lval* v);

/* Create a new number type lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Create a new error type lval */
lval* lval_err(char* x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err=malloc(strlen(x)+1);
  strcpy(v->err,x);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym=malloc(strlen(s)+1);
  strcpy(v->sym,s);
  return v;
}


lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {

  switch (v->type) {
    /* Do nothing special for number type */
    case LVAL_NUM: break;

    /* For Err or Sym free the string data */
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    /* If Sexpr then delete all elements inside */
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      /* Also free the memory allocated to contain the pointers */
      free(v->cell);
    break;
  }

  /* Free the memory allocated for the "lval" struct itself */
  free(v);
}


lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}


void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {

    /* Print Value contained within */
    lval_print(v->cell[i]);

    /* Don't print trailing space if last element */
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }


// lval eval_op(lval x, char* op, lval y) {
  
//   /* If either value is an error return it */
//   if (x.type == LVAL_ERR) { return x; }
//   if (y.type == LVAL_ERR) { return y; }
  
//   /* Otherwise do maths on the number values */
//   if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
//   if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
//   if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
//   if (strcmp(op, "/") == 0) {
//     /* If second operand is zero return error */
//     return lval_num(x.num / y.num);
//   }
//   if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
//   if (strcmp(op, "add") == 0) { return lval_num(x.num + y.num); }
//   if (strcmp(op, "sub") == 0) { return lval_num(x.num - y.num); }
//   if (strcmp(op, "mul") == 0) { return lval_num(x.num * y.num); }
//   if (strcmp(op, "div") == 0) { return lval_num(x.num / y.num); }
//   if (strcmp(op, "^") == 0) { return lval_num(pow(x.num,y.num)); }
//   if (strcmp(op, "min") == 0) { return lval_num(min(x.num,y.num)); }
//   if (strcmp(op, "max") == 0) { return lval_num(max(x.num,y.num)); }

  
//   return lval_err(LERR_BAD_OP);
// }

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ?
    lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {

  /* If Symbol or Number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* If root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); } 
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

  /* Fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}





int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Expression = mpc_new("expr");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Mylisp = mpc_new("mylisp");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                           \
    number : /-?[0-9]+/; \
    symbol    : '+' | '-' | '*' | '/' | '%' | /add/ | /mul/ | /sub/ | /div/ | '^' | /min/ | /max/;           \
    sexpr : '(' <expr>* ')' ;\
    expr      : <number> | <symbol> | <sexpr> ; \
    mylisp     : /^/ <expr>* /$/ ;                    \
  ",
  Number, Symbol, Sexpr,Expression , Mylisp);

  puts("myLisp Version 0.0.0.0.3");
  puts("Press Ctrl+c to Exit\n");

  while (1) {

  	char* input = readline("mylisp> ");

    add_history(input);

    mpc_result_t r;

	if (mpc_parse("<stdin>", input, Mylisp, &r)) {
	  /* On Success Print the AST */
	  // lval result = eval(r.output);
	  lval* x = lval_read(r.output);
	  lval_println(x);
	  lval_del(x);
	  mpc_ast_delete(r.output);

	} else {
	  /* Otherwise Print the Error */
	  mpc_err_print(r.error);
	  mpc_err_delete(r.error);
	}

    free(input);
  }

  mpc_cleanup(5, Number, Symbol,  Sexpr,Expression, Mylisp);
  return 0;
}