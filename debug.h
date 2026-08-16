#ifndef DEBUG_H_
#define DEBUG_H_

#include "csub.h"
void print_function(function_t *fn);
void dump_function(function_t *fn);
void dump_ast_tree(function_t *fn);
void dump_ast_node(node_t *node, char *label);
void dump_ast_asm(function_t *prog);

#endif
