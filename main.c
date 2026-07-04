#include "csub.h"

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  token_t *tok = tokenize(argv[1]);
  node_t *node = parse(tok);
  codegen(node);
  return 0;
}
