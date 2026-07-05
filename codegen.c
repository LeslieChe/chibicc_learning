#include "csub.h"

static int depth;

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


// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
// 获得变量的地址
static void gen_addr(node_t *node) {
  if (node->kind == ND_VAR) {
    int offset = (node->name - 'a' + 1) * 8;
    printf("  lea %d(%%rbp), %%rax\n", -offset);
    return;
  }

  error("not an lvalue");
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
            pop("%rdi"); // lhs 的地址在 rdi 中，rhs 的值在 rax 中
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
    if (node->kind == ND_EXPR_STMT) {
        gen_expr(node->lhs);
        return;
    }

    error("invalid statement");
}

void codegen(node_t *node)
{
    if (!node)
        return;

    printf("  .globl main\n");
    printf("main:\n");
    // Prologue
    printf("  push %%rbp\n");
    printf("  mov %%rsp, %%rbp\n");
    printf("  sub $208, %%rsp\n");

    for (node_t *n = node; n; n = n->next) {
        gen_stmt(n);
        assert(depth == 0);
    }

    printf("  mov %%rbp, %%rsp\n");
    printf("  pop %%rbp\n");
    printf("  ret\n");  // 返回值在 %rax 中
}
