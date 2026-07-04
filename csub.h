#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

typedef enum {
  TK_PUNCT, // Keywords or punctuators
  TK_NUM,   // Numeric literals
  TK_EOF,   // End-of-file markers
} token_kind_e;

// Token type
typedef struct token {
  token_kind_e kind; // Token kind
  struct token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *loc;      // Token location, first character of the token
  int len;        // Token length
}token_t;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(token_t *tok, char *fmt, ...);
bool equal(token_t *tok, char *op);
token_t *match_skip(token_t *tok, char *op);
token_t *tokenize(char *input);

//
// parse.c
//

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NEG, // unary -
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_EXPR_STMT, // Expression statement
  ND_NUM, // Integer
} node_kind_e;

// AST node type
typedef struct node {
  node_kind_e kind; // Node kind
  struct node *next;    // Next node
  struct node *lhs;     // Left-hand side
  struct node *rhs;     // Right-hand side
  int val;       // Used if kind == ND_NUM
}node_t;

node_t *parse(token_t *tok);

//
// codegen.c
//

void codegen(node_t *node);
