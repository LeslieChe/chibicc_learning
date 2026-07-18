#include "csub.h"

static int depth;

static int count(void) {
  static int i = 1;
  return i++;
}
static void push(void)
{
    printf("  push %%rax\n");
    depth++;
}

static void pop(char *arg)
{
    printf("  pop %s\n", arg);
    depth--;
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
static int align_to(int n, int align)
{
    return (n + align - 1) / align * align;
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
// 获得变量的地址
static void gen_addr(node_t *node)
{
    if (node->kind == ND_VAR) {
        printf("  lea %d(%%rbp), %%rax\n", node->var->offset);
        return;
    }
    error_tok(node->tok, "not an lvalue");
    
}

static void gen_expr(node_t *node)
{
    switch (node->kind) {
        case ND_NUM:
            printf("  mov $%d, %%rax\n", node->val);
            return;

        case ND_NEG:
            gen_expr(node->lhs);
            printf("  neg %%rax\n");
            return;

        case ND_VAR:
            gen_addr(node);
            // 把变量的值加载到寄存器 rax 中
            printf("  mov (%%rax), %%rax\n");
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            push();
            gen_expr(node->rhs);
            pop("%rdi");  // lhs 的地址在 rdi 中，rhs 的值在 rax 中
            printf("  mov %%rax, (%%rdi)\n");
            return;
    }

    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");  // rax 和 rdi 是两个通用寄存器，rax
                  // 用于存放左操作数，rdi 用于存放右操作数

    switch (node->kind) {
        case ND_ADD:
            printf("  add %%rdi, %%rax\n");  // rdi + rax -> rax
            return;
        case ND_SUB:
            printf("  sub %%rdi, %%rax\n");  // rdi - rax -> rax
            return;
        case ND_MUL:
            printf("  imul %%rdi, %%rax\n");  // rdi * rax -> rax
            return;
        case ND_DIV:
            printf("  cqo\n");  //  把 RAX 的符号位（第 63
                                //  位）直接复制到 RDX 的所有位中
            printf("  idiv %%rdi\n");  // 商存放在 %rax 中。余数存放在
                                       // %rdx 中。
            return;
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
            printf("  cmp %%rdi, %%rax\n");  // rax - rdi

            if (node->kind == ND_EQ)
                printf("  sete %%al\n");
            else if (node->kind == ND_NE)
                printf("  setne %%al\n");
            else if (node->kind == ND_LT)  // <
                printf("  setl %%al\n");
            else if (node->kind == ND_LE)  // <=
                printf("  setle %%al\n");

            printf("  movzbq %%al, %%rax\n");
            return;
    }

    error("invalid expression");
}

static void gen_stmt(node_t *node)
{
    switch (node->kind) {
        case ND_IF: {
            int c = count();
            gen_expr(node->cond);
            printf("  cmp $0, %%rax\n");  // rax - 0
            printf("  je  .L.else.%d\n", c);
            gen_stmt(node->then);
            printf("  jmp .L.end.%d\n", c);
            printf(".L.else.%d:\n", c);
            if (node->els)
                gen_stmt(node->els);
            printf(".L.end.%d:\n", c);
            return;
        }

        case ND_FOR: {  // and while are both implemented as for loops
            int c = count();
            if (node->init)
                gen_stmt(node->init);
            printf(".L.begin.%d:\n", c);
            if (node->cond) {
                gen_expr(node->cond);
                printf("  cmp $0, %%rax\n");
                printf("  je  .L.end.%d\n", c);
            }
            gen_stmt(node->then);
            if (node->inc)
                gen_expr(node->inc);
            printf("  jmp .L.begin.%d\n", c);
            printf(".L.end.%d:\n", c);
            return;
        }

        case ND_BLOCK:
            for (node_t *n = node->body; n; n = n->next) gen_stmt(n);
            return;
        case ND_RETURN:
            gen_expr(node->lhs);
            printf("  jmp .L.return\n");
            return;
        case ND_EXPR_STMT:
            gen_expr(node->lhs);
            return;
    }

    error("invalid statement");
}

/*
SysV AMD64 ABI 规定：调用 call 指令之前，rsp 必须是 16 的倍数。
这样进入被调用函数后，返回地址压栈后，函数内的 rsp 变成 8 mod 16，
再做 push rbp/sub 之后可以正确恢复到 16 对齐。
16 字节对齐对 SSE/AVX 以及某些库函数的内存访问很重要，
很多指令（例如 movdqa）要求对齐，或者至少性能更好。
*/
// Assign offsets to local variables.
static void assign_lvar_offsets(function_t *prog)
{
    int offset = 0;
    for (obj_t *var = prog->locals; var; var = var->next) {
        offset += 8;
        var->offset = -offset;
    }
    prog->stack_size = align_to(offset, 16);
}

void codegen(function_t *prog)
{
    if (!prog)
        return;

    assign_lvar_offsets(prog);
    printf("  .globl main\n");
    printf("main:\n");
    // Prologue
    printf("  push %%rbp\n");
    printf("  mov %%rsp, %%rbp\n");
    printf("  sub $%d, %%rsp\n", prog->stack_size);

    gen_stmt(prog->body);
    assert(depth == 0);

    printf(".L.return:\n");
    printf("  mov %%rbp, %%rsp\n");
    printf("  pop %%rbp\n");
    printf("  ret\n");  // 返回值在 %rax 中
}
