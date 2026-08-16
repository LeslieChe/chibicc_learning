#include "debug.h"

// 一共 316 个提交
int main(int argc, char **argv)
{
    bool ast_asm = (argc == 3) && !strcmp(argv[1], "--ast-asm");
    bool ast = (argc == 3) && !strcmp(argv[1], "--ast");
    if ((!ast_asm && !ast && argc != 2) || (ast_asm && argc != 3) ||
        (ast && argc != 3))
        error("usage: %s [--ast-asm | --ast] <source>", argv[0]);

    token_t *tok = tokenize(argv[ast_asm ? 2 : (ast ? 2 : 1)]);
    function_t *prog = parse(tok);
    if (ast_asm)
        dump_ast_asm(prog);
    else {
        codegen(prog);
        if (ast)
        dump_ast_tree(prog);
    }

    return 0;
}
