#include "csub.h"

static int depth;
/*
System V AMD64 ABI（Application Binary
Interface）规定了函数调用的约定，
包括参数传递、返回值处理、栈帧布局等。
根据该规范，函数调用时，前六个整数或指针类型的参数会通过寄存器传递，
而不是通过栈。这些寄存器分别是：
1. %rdi - 用于传递第一个参数
2. %rsi - 用于传递第二个参数
3. %rdx - 用于传递第三个参数
4. %rcx - 用于传递第四个参数
5. %r8  - 用于传递第五个参数
6. %r9  - 用于传递第六个参数
如果函数有超过六个参数，剩余的参数会通过栈传递。
*/
static char *arg_reg[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static function_t *current_fn;
static void gen_expr(node_t *node);
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
    switch (node->kind) {
        case ND_VAR:
            printf("  lea %d(%%rbp), %%rax\n", node->var->offset);
            return;
        case ND_DEREF:  // &*x = x; &*抵消
            gen_expr(node->lhs);
            return;
    }
    error_tok(node->tok, "not an lvalue");
}

// Load a value from where %rax is pointing to.
// 把 rax 指向的值加载到 rax 中
// 如果 rax
// 指向的是一个数组，则什么也不做，因为数组不能整体加载到寄存器中
static void load(type_t *ty)
{
    if (ty->kind == TY_ARRAY) {
        // If it is an array, do not attempt to load a value to the
        // register because in general we can't load an entire array
        // to a register. As a result, the result of an evaluation of
        // an array becomes not the array itself but the address of
        // the array. This is where "array is automatically converted
        // to a pointer to the first element of the array in C"
        // occurs.
        // printf("  # array type, do nothing\n");
        return;
    }
  //  printf("  # load value \n");
    printf("  mov (%%rax), %%rax\n");
}

// 地址在栈上，出栈，再把 rax 的值存放到这个地址中
// Store %rax to an address that the stack top is pointing to.
static void store(void)
{
    pop("%rdi");
    printf("  mov %%rax, (%%rdi)\n");
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

        // 右值
        case ND_VAR: // 从内存中加载变量的值到寄存器 rax 中
            gen_addr(node); //  step1 把变量的地址加载到寄存器 rax 中
            load(node->ty); //  step2 把 rax 指向的值加载到 rax 中
            // 如果 node->ty->kind == TY_ARRAY，则什么也不做，因为数组不能整体加载到寄存器中
            return;

        case ND_DEREF: // 右值
            gen_expr(node->lhs); // 得到一个地址在 rax 中
            load(node->ty); // 把 rax 指向的值加载到 rax 中
            return;

        case ND_ADDR:
            gen_addr(node->lhs);
            return;

        case ND_ASSIGN:  // 对左值特殊处理
            gen_addr(node->lhs);
            push(); // 把左值的地址压栈
            gen_expr(node->rhs); // 右值的值加载到 rax 中
            store(); // 出栈，得到一个地址，把 rax 的值存放到这个地址中
            return;

        case ND_FUNCALL: {
            int nargs = 0;
            for (node_t *arg = node->args; arg; arg = arg->next) {
                gen_expr(arg);
                push();
                nargs++;
            }

            for (int i = nargs - 1; i >= 0; i--) 
                pop(arg_reg[i]);

            printf("  mov $0, %%rax\n");  // 似乎没有必要
            printf("  call %s\n", node->funcname);
            return;
        }
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

     error_tok(node->tok, "invalid expression");
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
            for (node_t *n = node->body; n; n = n->next) 
                gen_stmt(n);
            return;
        case ND_RETURN:
            gen_expr(node->lhs);         
            printf("  jmp .L.return.%s\n", current_fn->name);
            return;
        case ND_EXPR_STMT:
            gen_expr(node->lhs);
            return;
    }

     error_tok(node->tok, "invalid statement");
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
    for (function_t *fn = prog; fn; fn = fn->next) {
        int offset = 0;
        for (obj_t *var = fn->locals; var; var = var->next) {
            offset += var->ty->size;
            var->offset = -offset;
        }
        fn->stack_size = align_to(offset, 16);
    }
}


void codegen(function_t *prog)
{
    assign_lvar_offsets(prog);

    for (function_t *fn = prog; fn; fn = fn->next) {
        printf("  .globl %s\n", fn->name);
        printf("%s:\n", fn->name);
        current_fn = fn;

        // Prologue
        printf("  push %%rbp\n");
        printf("  mov %%rsp, %%rbp\n");
        printf("  sub $%d, %%rsp\n", fn->stack_size);

        // Save passed-by-register arguments to the stack
        // 把寄存器中的参数保存到栈中，方便后续使用
        int i = 0;
        // 这里形式参数不能超过 6 个，否则会越界！
        for (obj_t *var = fn->params; var; var = var->next)
            printf("  mov %s, %d(%%rbp)\n", arg_reg[i++],
                   var->offset);

        // Emit code
        gen_stmt(fn->body);
        assert(depth == 0);

        // Epilogue
        printf(".L.return.%s:\n", fn->name);
        printf("  mov %%rbp, %%rsp\n");
        printf("  pop %%rbp\n");
        printf("  ret\n");
    }
}

void codegen_node(function_t *fn, node_t *node)
{
    assign_lvar_offsets(fn);
    current_fn = fn;
    gen_expr(node);
    assert(depth == 0);
}
