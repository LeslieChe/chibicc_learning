#include "csub.h"

// copy from 725bad
// 一共 316 个提交
int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  token_t *tok = tokenize(argv[1]);
  function_t *prog = parse(tok);
  codegen(prog);
  return 0;
}
