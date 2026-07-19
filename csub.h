#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct type type_t; // forward declaration

//
// tokenize.c
//

typedef enum {
    TK_IDENT,  // Identifiers, single character variable names
    TK_PUNCT,  // Keywords or punctuators
    TK_KEYWORD, // Keywords
    TK_NUM,    // Numeric literals
    TK_EOF,    // End-of-file markers
} token_kind_e;

// Token type
typedef struct token
{
    token_kind_e kind;   // Token kind
    struct token *next;  // Next token
    int val;             // If kind is TK_NUM, its value
    char *loc;  // Token location, first character of the token
    int len;    // Token length
} token_t;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(token_t *tok, char *fmt, ...);
bool equal(token_t *tok, char *op);
token_t *match_skip(token_t *tok, char *op);
bool consume(token_t **rest, token_t *tok, char *str);
token_t *tokenize(char *input);

//
// parse.c
//

typedef enum {
    ND_ADD,        // +
    ND_SUB,        // -
    ND_MUL,        // *
    ND_DIV,        // /
    ND_NEG,        // unary -
    ND_EQ,         // ==
    ND_NE,         // !=
    ND_LT,         // <
    ND_LE,         // <=
    ND_ASSIGN,    // =
    ND_ADDR,      // unary &
    ND_DEREF,     // unary *
    ND_RETURN,    // "return"
    ND_IF,        // "if"
    ND_FOR,       // // "for" or "while"
    ND_BLOCK,     // { ... }
    ND_EXPR_STMT,  // Expression statement
    ND_VAR,       // Variable
    ND_NUM,        // Integer
} node_kind_e;

// AST node type
typedef struct obj obj_t; // forward declaration

typedef struct node
{
    node_kind_e kind;   // Node kind
    struct node *next;  // Next node
    type_t *ty;
    token_t *tok;    // Representative token   暂时不理解
    struct node *lhs;   // Left-hand side
    struct node *rhs;   // Right-hand side

    // "if" or "for" statement
    struct node *cond;  // if or for condition
    struct node *then;  // if or for body
    struct node *els;
    struct node *init;  // for init statement
    struct node *inc;   // for increment statement

    struct node *body;  // Block
    obj_t *var;         // Used if kind == ND_VAR
    int val;            // Used if kind == ND_NUM
} node_t;

// Local variable

struct obj {
  struct obj *next;
  char *name; // Variable name
  type_t *ty; // Type
  int offset; // Offset from RBP
};

// Function
typedef struct function {
  node_t *body;
  obj_t *locals;
  int stack_size;
}function_t;

function_t *parse(token_t *tok);


//
// type.c
//

typedef enum {
  TY_INT,
  TY_PTR,
} type_kind_e;

struct type {
  type_kind_e kind;
  struct type *base;
   // Declaration
  token_t *name; // 暂时不理解
};

extern struct type *ty_int;

bool is_integer(struct type *ty);
type_t *pointer_to(type_t *base);
void add_type(struct node *node);

//
// codegen.c
//

void codegen(function_t *prog);
