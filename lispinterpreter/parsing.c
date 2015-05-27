//Includes
#include<stdio.h>
#include<stdlib.h>
#include <math.h>
#include"mpc.h"


//Macros
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define LASSERT(args,cond,err)\
   if(!(cond)){ lval_del(args); return lval_err(err);}



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



//Declarations
struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;


enum { LVAL_NUM, LVAL_ERR, LVAL_SYM,LVAL_FUN ,LVAL_SEXPR , LVAL_QEXPR};

typedef lval* (*lbuiltin)(lenv*,lval*);

struct lval{
  int type;
  long num;

  char* err;
  char* sym;

  lbuiltin fun;

  int count;
  lval** cell;
} ;

void lval_print(lval* v);

//Constructors and destructors


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

/* Create a new symbol type lval */
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym=malloc(strlen(s)+1);
  strcpy(v->sym,s);
  return v;
}

/* Create a new sexpr type lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* Create a new qexpr type lval */
lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_fun(lbuiltin func){
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->fun =func;
  return v;
}

//LVAL Functions


//Delete and free memory
void lval_del(lval* v) {

  switch (v->type) {
    /* Do nothing special for number type */
    case LVAL_NUM: break;

    /* For Err or Sym free the string data */
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    case LVAL_FUN: break;
    /* If Sexpr then delete all elements inside */
    case LVAL_QEXPR:
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

// Add cell
lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

//Copy lvals
lval* lval_copy(lval* v) {
  
  lval* x = malloc(sizeof(lval));
  x->type = v->type;
  
  switch (v->type) {
    
    /* Copy Functions and Numbers Directly */
    case LVAL_FUN: x->fun = v->fun; break;
    case LVAL_NUM: x->num = v->num; break;
    
    /* Copy Strings using malloc and strcpy */
    case LVAL_ERR:
      x->err = malloc(strlen(v->err) + 1);
      strcpy(x->err, v->err); break;
    
    case LVAL_SYM:
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym); break;

    /* Copy Lists by copying each sub-expression */
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);
      for (int i = 0; i < x->count; i++) {
        x->cell[i] = lval_copy(v->cell[i]);
      }
    break;
  }
  
  return x;
}
// Pop a cell
lval* lval_pop(lval* v, int i) {
  /* Find the item at "i" */
  lval* x = v->cell[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&v->cell[i], &v->cell[i+1],
    sizeof(lval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

// Pop and delete the list
lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}



//Print Functions
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


// Print values
void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    case LVAL_FUN:   printf("<function>"); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }


//Read functions
//Read Numbers
lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ?
    lval_num(x) : lval_err("invalid number");
}


//Read values 
lval* lval_read(mpc_ast_t* t) {

  /* If Symbol or Number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* If root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); } 
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

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



lval* lval_eval(lval* v);

//Builtin functions
lval* builtin(lval* a, char* op);
lval* builtin_op(lval* a, char* op);

//Head function
lval* builtin_head(lval* a){
  LASSERT(a, a->count==1,"Error : Function 'head' passed too many arguments");
  LASSERT(a, a->cell[0]->type==LVAL_QEXPR,"Error : Function 'head has incorrect type!'");
  LASSERT(a, a->cell[0]->count!=0,"Error : Function head passed an empty expression");

  lval* x = lval_take(a,0);
  while(x->count >1){lval_del(lval_pop(x,1));}
  return x;
}

//Count function
lval* builtin_len(lval* a){
  LASSERT(a, a->count==1,"Error : Function 'head' passed too many arguments");
  LASSERT(a, a->cell[0]->type==LVAL_QEXPR,"Error : Function 'head has incorrect type!'");
  LASSERT(a, a->cell[0]->count!=0,"Error : Function head passed an empty expression");

  lval* x = lval_take(a,0);
  lval* count=lval_num(x->count);
  lval_del(x);
  return count;
}

//Tail function
lval* builtin_tail(lval* a){
  LASSERT(a, a->count==1,"Error : Function 'tail' passed too many arguments");
  LASSERT(a, a->cell[0]->type==LVAL_QEXPR,"Error : Function 'tail' has incorrect type!'");
  LASSERT(a, a->cell[0]->count!=0,"Error : Function tail passed an empty expression");

  lval* x = lval_pop(a,0);
  lval_del(lval_pop(x, 0));
  return x;
}

lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* lval_join(lval* x, lval* y) {

  /* For each cell in 'y' add it to 'x' */
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  /* Delete the empty 'y' and return 'x' */
  lval_del(y);  
  return x;
}

lval* builtin_join(lval* a) {

  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
      "Function 'join' passed incorrect type.");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* builtin_cons(lval* a)
{
  lval* x=lval_qexpr();
  x=lval_add(x,lval_pop(a, 0));
  x=lval_join(x,lval_pop(a,0));
  return x;
}

lval* builtin_eval(lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'eval' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed incorrect type!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}



//Evaluate Sexpression
lval* lval_eval_sexpr(lval* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  /* Error Checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  /* Empty Expression */
  if (v->count == 0) { return v; }

  /* Single Expression */
  if (v->count == 1) { return lval_take(v, 0); }

  /* Ensure First Element is Symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  /* Call builtin with operator */
  lval* result = builtin(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v) {
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  /* All other lval types remain the same */
  return v;
}


//Function which carries the actual calculation
lval* builtin_op(lval* a, char* op) {
  
  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }
  
  /* Pop the first element */
  lval* x = lval_pop(a, 0);

  /* If no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  /* While there are still elements remaining */
  while (a->count > 0) {

    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero!"); break;
      }
      x->num /= y->num;
    }
      if (strcmp(op, "%") == 0) {  x->num %= y->num; }
	  if (strcmp(op, "add") == 0) {  x->num += y->num; }
	  if (strcmp(op, "sub") == 0) {  x->num -= y->num; }
	  if (strcmp(op, "mul") == 0) {  x->num *= y->num; }
	  if (strcmp(op, "div") == 0) {  x->num /= y->num; }
	  if (strcmp(op, "^") == 0) {  x->num=pow(x->num,y->num); }
	  if (strcmp(op, "min") == 0) {  x->num=min(x->num,y->num); }
	  if (strcmp(op, "max") == 0) {  x->num=max(x->num,y->num); }



    lval_del(y);
  }

  lval_del(a); return x;
}

lval* builtin(lval* a, char* func) {
  if (strcmp("list", func) == 0) { return builtin_list(a); }
  if (strcmp("head", func) == 0) { return builtin_head(a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(a); }
  if (strcmp("join", func) == 0) { return builtin_join(a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(a); }
  if (strcmp("len", func) == 0) { return builtin_len(a); }
  if (strcmp("cons", func) == 0) { return builtin_cons(a); }
  if (strstr("+-/*", func)) { return builtin_op(a, func); }
  lval_del(a);
  return lval_err("Unknown Function!");
}

//Main Function
int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Expression = mpc_new("expr");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Mylisp = mpc_new("mylisp");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                           \
    number : /-?[0-9]+/; \
    symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/  ;           \
    sexpr : '(' <expr>* ')' ;\
    qexpr : '{' <expr>* '}' ;\
    expr      : <number> | <symbol> | <sexpr> | <qexpr>; \
    mylisp     : /^/ <expr>* /$/ ;                    \
  ",
  Number, Symbol, Sexpr,Qexpr,Expression , Mylisp);

  puts("myLisp Version 0.0.0.0.5");
  puts("Press Ctrl+c to Exit\n");

  while (1) {

  	char* input = readline("mylisp> ");

    add_history(input);

    mpc_result_t r;

	if (mpc_parse("<stdin>", input, Mylisp, &r)) {
	  /* On Success Print the AST */
	  // lval result = eval(r.output);
	  lval* x = lval_eval(lval_read(r.output));
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

  mpc_cleanup(5, Number, Symbol, Sexpr, Qexpr, Expression, Mylisp);
  return 0;
}