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

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { \
    lval* err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(args); \
    return err; \
  }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);


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

  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;

  int count;
  lval** cell;
};

struct lenv{
  lenv* par;
  int count;
  char** syms;
  lval** vals;
};


void lval_print(lval* v);
lval* lval_err(char* fmt, ...);

//Constructors and destructors for lval


/* Create a new number type lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Create a new error type lval */
lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  /* Create a va list and initialize it */
  va_list va;
  va_start(va, fmt);

  /* Allocate 512 bytes of space */
  v->err = malloc(512);

  /* printf the error string with a maximum of 511 characters */
  vsnprintf(v->err, 511, fmt, va);

  /* Reallocate to number of bytes actually used */
  v->err = realloc(v->err, strlen(v->err)+1);

  /* Cleanup our va list */
  va_end(va);

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
  v->builtin =func;
  return v;
}

lval* lval_lambda(lval* formals, lval* body) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;

  /* Set Builtin to Null */
  v->builtin = NULL;

  /* Build new environment */
  v->env = lenv_new();

  /* Set Formals and Body */
  v->formals = formals;
  v->body = body;
  return v;  
}

char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
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
    case LVAL_FUN: 
        if (!v->builtin) {
          lenv_del(v->env);
          lval_del(v->formals);
          lval_del(v->body);
        }
        break;
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
    case LVAL_FUN:
      if (v->builtin) {
        x->builtin = v->builtin;
      } else {
        x->builtin = NULL;
        x->env = lenv_copy(v->env);
        x->formals = lval_copy(v->formals);
        x->body = lval_copy(v->body);
      }
      break;
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
    case LVAL_FUN:   
      if (v->builtin) {
        printf("<builtin>");
      } else {
        printf("(\\ "); lval_print(v->formals);
        putchar(' '); lval_print(v->body); putchar(')');
      }
      break;
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



lval* lval_eval(lenv* ,lval*);

//Builtin functions
lval* builtin(lenv* , lval* , char* );
lval* builtin_op(lenv*, lval* , char* );

//Head function
lval* builtin_head(lenv* e,lval* a){
  LASSERT_NUM("head", a, 1);
  LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("head", a, 0);

  lval* x = lval_take(a,0);
  while(x->count >1){lval_del(lval_pop(x,1));}
  return x;
}

//Count function
lval* builtin_len(lenv* e,lval* a){
  LASSERT_NUM("len", a, 1);
  LASSERT_TYPE("len", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("len", a, 0);
  lval* x = lval_take(a,0);
  lval* count=lval_num(x->count);
  lval_del(x);
  return count;
}

//Tail function
lval* builtin_tail(lenv* e,lval* a){
  LASSERT_NUM("tail", a, 1);
  LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("tail", a, 0);

  lval* x = lval_pop(a,0);
  lval_del(lval_pop(x, 0));
  return x;
}

lval* builtin_list(lenv* e,lval* a) {
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

lval* builtin_join(lenv* e,lval* a) {

  for (int i = 0; i < a->count; i++) {
  LASSERT_TYPE("join", a, i, LVAL_QEXPR);
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* builtin_cons(lenv* e,lval* a)
{
  lval* x=lval_qexpr();
  x=lval_add(x,lval_pop(a, 0));
  x=lval_join(x,lval_pop(a,0));
  return x;
}

lval* builtin_eval(lenv* e,lval* a) {
  
  LASSERT_NUM("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e,x);
}

lval* builtin_lambda(lenv* e , lval*a)
{
  LASSERT_NUM("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
      "Cannot define non-symbol. Got %s, Expected %s.",
      ltype_name(a->cell[0]->cell[i]->type),ltype_name(LVAL_SYM));
  }

  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);
  
  return lval_lambda(formals, body);



}



//Evaluate Sexpression
lval* lval_eval_sexpr(lenv* e,lval* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e,v->cell[i]);
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
  if (f->type != LVAL_FUN) {
    lval_del(f); lval_del(v);
    return lval_err(
      "S-Expression starts with incorrect type. "
      "Got %s, Expected %s.",
      ltype_name(f->type), ltype_name(LVAL_FUN));
  }

  /* Call builtin with operator */
  lval* result = f->builtin(e,v);
  lval_del(f);
  return result;
}

//Constructors and destructors for lenv

lenv* lenv_new(void)
{
  lenv* e = malloc(sizeof(lenv*));
  e->count=0;
  e->par = NULL;
  e->syms=NULL;
  e->vals=NULL;
  return e; 
}

void lenv_del(lenv* v)
{
  for (int i = 0; i < v->count; i++)
  {
    free(v->syms[i]);
    lval_del(v->vals[i]);
  }

  free(v->syms);
  free(v->vals);
  free(v);
}

//Get from Environment
lval* lenv_get(lenv* e, lval* k)
{
  for (int i = 0; i < e->count; i++)
  {
    if(strcmp(e->syms[i],k->sym)==0)
    {
      return lval_copy(e->vals[i]);
    }
  }
  if (e->par) {
    return lenv_get(e->par, k);
  } else {
    return lval_err("Unbound Symbol '%s'", k->sym);
  }
  
}

void lenv_put(lenv* e,lval* k,lval*v)
{
  for (int i = 0; i < e->count; i++)
  {
    if(strcmp(e->syms[i],k->sym)==0)
    {
      lval_del(e->vals[i]);
      e->vals[i]=lval_copy(v);
      return;
    }
  }


  e->count++;
  e->syms=realloc(e->syms,sizeof(char*)*e->count);
  e->vals=realloc(e->vals,sizeof(lval*)*e->count);

  e->vals[e->count-1]=lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1],k->sym);
}

void lenv_def(lenv* e, lval* k, lval* v) {
  /* Iterate till e has no parent */
  while (e->par) { e = e->par; }
  /* Put value in e */
  lenv_put(e, k, v);
}

lenv* lenv_copy(lenv* e) {
  lenv* n = malloc(sizeof(lenv));
  n->par = e->par;
  n->count = e->count;
  n->syms = malloc(sizeof(char*) * n->count);
  n->vals = malloc(sizeof(lval*) * n->count);
  for (int i = 0; i < e->count; i++) {
    n->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(n->syms[i], e->syms[i]);
    n->vals[i] = lval_copy(e->vals[i]);
  }
  return n;
}

lval* lval_eval(lenv* e,lval* v) {

  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e,v); }
  /* All other lval types remain the same */
  return v;
}

lval* builtin_var(lenv* e, lval* a,char* func) {
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
  
  /* First argument is symbol list */
  lval* syms = a->cell[0];

  /* Ensure all elements of first list are symbols */
  for (int i = 0; i < syms->count; i++) {
  LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
      "Function 'def' cannot define non-symbol. "
      "Got %s, Expected %s.",
      ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
  }
  
  /* Check correct number of symbols and values */
  LASSERT(a, (syms->count == a->count-1),
    "Function 'def' passed too many arguments for symbols. "
    "Got %i, Expected %i.",
    syms->count, a->count-1);

  /* Assign copies of values to symbols */
  for (int i = 0; i < syms->count; i++) {
    if (strcmp(func, "def") == 0) {
      lenv_def(e, syms->cell[i], a->cell[i+1]);
    }
    
    if (strcmp(func, "=")   == 0) {
      lenv_put(e, syms->cell[i], a->cell[i+1]);
    } 
  }

  lval_del(a);
  return lval_sexpr();
}


lval* builtin_def(lenv* e, lval* a) {
  return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a) {
  return builtin_var(e, a, "=");
}


lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}

lval* builtin_pow(lenv* e, lval* a) {
  return builtin_op(e, a, "^");
}

lval* builtin_mod(lenv* e, lval* a) {
  return builtin_op(e, a, "%");
}

lval* builtin_min(lenv* e, lval* a) {
  return builtin_op(e, a, "min");
}

lval* builtin_max(lenv* e, lval* a) {
  return builtin_op(e, a, "max");
}



//Function which carries the actual calculation
lval* builtin_op(lenv *e , lval* a, char* op) {
  
  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
     LASSERT_TYPE(op, a, i, LVAL_NUM);
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

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k); lval_del(v);
}


void lenv_add_builtins(lenv* e) { 
 
  /* Define variables */
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "=",   builtin_put);

  /* List Functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "%", builtin_mod);
  lenv_add_builtin(e, "^", builtin_pow);
  lenv_add_builtin(e, "min", builtin_min);
  lenv_add_builtin(e, "max", builtin_max);
  lenv_add_builtin(e, "add", builtin_add);
  lenv_add_builtin(e, "sub", builtin_sub);
  lenv_add_builtin(e, "mul", builtin_mul);
  lenv_add_builtin(e, "div", builtin_div);


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
    symbol    : /[a-zA-Z0-9_+\\^\\-*\\/\\\\=<>!&]+/  ;           \
    sexpr : '(' <expr>* ')' ;\
    qexpr : '{' <expr>* '}' ;\
    expr      : <number> | <symbol> | <sexpr> | <qexpr>; \
    mylisp     : /^/ <expr>* /$/ ;                    \
  ",
  Number, Symbol, Sexpr,Qexpr,Expression , Mylisp);

  puts("myLisp Version 0.0.0.0.7");
  puts("Press Ctrl+c to Exit\n");

  lenv* e = lenv_new();
  lenv_add_builtins(e);

  while (1) {

  	char* input = readline("mylisp> ");

    add_history(input);

    mpc_result_t r;

  	if (mpc_parse("<stdin>", input, Mylisp, &r)) {
  	  /* On Success Print the AST */
  	  // lval result = eval(r.output);
  	  lval* x = lval_eval(e,lval_read(r.output));
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

  lenv_del(e);

  mpc_cleanup(5, Number, Symbol, Sexpr, Qexpr, Expression, Mylisp);
  return 0;
}