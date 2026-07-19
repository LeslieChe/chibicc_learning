#include "csub.h"

type_t *ty_int = &(type_t){TY_INT};

bool is_integer(type_t *ty) {
  return ty->kind == TY_INT;
}

type_t *pointer_to(type_t *base) {
  type_t *ty = calloc(1, sizeof(type_t));
  ty->kind = TY_PTR;
  ty->base = base;
  return ty;
}

// 递归
// 自动推断类型
// 我认为此函数非常巧妙
void add_type(node_t *node) {
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

  switch (node->kind) {
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_NEG:
  case ND_ASSIGN:
    node->ty = node->lhs->ty;
    return;
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_VAR: // 目前只有整型变量
  case ND_NUM:
    node->ty = ty_int;
    return;
  case ND_ADDR:
    node->ty = pointer_to(node->lhs->ty);
    return;
  case ND_DEREF:
    if (node->lhs->ty->kind == TY_PTR)
      node->ty = node->lhs->ty->base;
    else // 只能对指针类型解引用，如果不是指针，那么认为结果是整型？不理解
      node->ty = ty_int;
    return;
  }
}