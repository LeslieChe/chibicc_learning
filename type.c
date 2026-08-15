#include "csub.h"

type_t *ty_int = &(type_t){TY_INT, 8};

bool is_integer(type_t *ty) { return ty->kind == TY_INT; }

type_t *copy_type(type_t *ty) {
  type_t *ret = calloc(1, sizeof(type_t));
  *ret = *ty;
  return ret;
}

type_t *pointer_to(type_t *base)
{
    type_t *ty = calloc(1, sizeof(type_t));
    ty->kind = TY_PTR;
    ty->size = 8; // 指针占 8 个字节
    ty->base = base;
    return ty;
}

type_t *func_type(type_t *return_ty)
{
    type_t *ty = calloc(1, sizeof(type_t));
    ty->kind = TY_FUNC;
    ty->return_ty = return_ty;
    return ty;
}


// 数组的元素类型是 base，元素个数是 len
type_t *array_of(type_t *base, int len) {
  type_t *ty = calloc(1, sizeof(type_t));
  ty->kind = TY_ARRAY;
  ty->size = base->size * len;
  ty->base = base;
  ty->array_len = len;
  return ty;
}

// 递归
// 自动推断类型
// 我认为此函数非常巧妙
void add_type(node_t *node)
{
    if (!node || node->ty)
        return;

    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->init);
    add_type(node->inc);

    for (node_t *n = node->body; n; n = n->next) 
        add_type(n);
    for (node_t *n = node->args; n; n = n->next)
        add_type(n);    
    switch (node->kind) {
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_NEG:
            node->ty = node->lhs->ty;
            node->debug_info = "lhs->ty";
            return;
        case ND_ASSIGN:
            if (node->lhs->ty->kind == TY_ARRAY)
                // 数组类型不能作为左值
                error_tok(node->lhs->tok, "not an lvalue");
            node->ty = node->lhs->ty;
            node->debug_info = "lhs->ty";
            return;
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_NUM:
        case ND_FUNCALL:  // 函数返回值是整型
            node->debug_info = "ty_int";
            node->ty = ty_int;
            return;
        case ND_VAR:
            node->ty = node->var->ty;
            node->debug_info = "var->ty";
            return;
        case ND_ADDR:
            if (node->lhs->ty->kind == TY_ARRAY) {
                // 这里不理解
                // 对数组取地址，得到的是指向数组首元素的指针
                node->ty = pointer_to(node->lhs->ty->base);
                 node->debug_info = "pointer_to(node->lhs->ty->base)";
            }
            else{
                node->ty = pointer_to(node->lhs->ty);
                node->debug_info = "pointer_to(node->lhs->ty)";
            }
                
            return; 
        case ND_DEREF:
        // 对于指针和数组，node->lhs->ty->base 非空
        // 对于整型、函数，node->lhs->ty->base 为空
            if (!node->lhs->ty->base) {
                error_tok(node->tok, "invalid pointer dereference\n");
            }
               
            node->ty = node->lhs->ty->base;
             node->debug_info = "lhs->ty->base";
            return;
    }
}

