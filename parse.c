#include "csub.h"

static node_t *expr(token_t **rest, token_t *tok);
static node_t *equality(token_t **rest, token_t *tok);
static node_t *relational(token_t **rest, token_t *tok);
static node_t *add(token_t **rest, token_t *tok);
static node_t *mul(token_t **rest, token_t *tok);
static node_t *unary(token_t **rest, token_t *tok);
static node_t *primary(token_t **rest, token_t *tok);

static node_t *new_node(node_kind_e kind)
{
    node_t *node = calloc(1, sizeof(node_t));
    node->kind = kind;
    return node;
}

static node_t *new_binary(node_kind_e kind, node_t *lhs, node_t *rhs)
{
    node_t *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static node_t *new_unary(node_kind_e kind, node_t *expr)
{
    node_t *node = new_node(kind);

    // 一元仅有一个操作数，存放在 lhs 中
    node->lhs = expr;
    return node;
}

static node_t *new_num(int val)
{
    node_t *node = new_node(ND_NUM);

    // ND_NUM 节点的值存放在 val 中
    node->val = val;
    return node;
}

// expr := equality
static node_t *expr(token_t **rest, token_t *tok)
{
    return equality(rest, tok);
}

// equality := relational ("==" relational | "!=" relational)*
static node_t *equality(token_t **rest, token_t *tok)
{
    node_t *node = relational(&tok, tok);

    for (;;) {
        if (equal(tok, "==")) {
            node =
                new_binary(ND_EQ, node, relational(&tok, tok->next));
            continue;
        }

        if (equal(tok, "!=")) {
            node =
                new_binary(ND_NE, node, relational(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

// relational := add ("<" add | "<=" add | ">" add | ">=" add)*
// add 是加性表达式
static node_t *relational(token_t **rest, token_t *tok)
{
    node_t *node = add(&tok, tok);

    for (;;) {
        if (equal(tok, "<")) {
            node = new_binary(ND_LT, node, add(&tok, tok->next));
            continue;
        }

        if (equal(tok, "<=")) {
            node = new_binary(ND_LE, node, add(&tok, tok->next));
            continue;
        }

        if (equal(tok, ">")) {
            node = new_binary(ND_LT, add(&tok, tok->next), node);
            continue;
        }

        if (equal(tok, ">=")) {
            node = new_binary(ND_LE, add(&tok, tok->next), node);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// add := mul ("+" mul | "-" mul)*
// mul 是乘性表达式
static node_t *add(token_t **rest, token_t *tok)
{
    node_t *node = mul(&tok, tok);

    for (;;) {
        if (equal(tok, "+")) {
            node = new_binary(ND_ADD, node, mul(&tok, tok->next));
            continue;
        }

        if (equal(tok, "-")) {
            node = new_binary(ND_SUB, node, mul(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

// mul := unary ("*" unary | "/" unary)*
static node_t *mul(token_t **rest, token_t *tok)
{
    node_t *node = unary(&tok, tok);

    for (;;) {
        if (equal(tok, "*")) {
            node = new_binary(ND_MUL, node, unary(&tok, tok->next));
            continue;
        }

        if (equal(tok, "/")) {
            node = new_binary(ND_DIV, node, unary(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

// unary = ("+" | "-") unary
//       | primary
static node_t *unary(token_t **rest, token_t *tok)
{
    if (equal(tok, "+"))
        return unary(rest, tok->next); // 递归

    if (equal(tok, "-"))
        return new_unary(ND_NEG, unary(rest, tok->next)); // 递归

    return primary(rest, tok);
}

// primary = "(" expr ")" | num
static node_t *primary(token_t **rest, token_t *tok)
{
    if (equal(tok, "(")) {
        node_t *node = expr(rest, tok->next);
        *rest = match_skip(*rest, ")");
        return node;
    }

    if (tok->kind == TK_NUM) {
        node_t *node = new_num(tok->val);
        *rest = tok->next;
        return node;
    }

    error_tok(tok, "expected an expression");
}

node_t *parse(token_t *tok)
{
    // expr 是入口函数，解析表达式
    // 测试的输入必须是表达式
    node_t *node = expr(&tok, tok);
    if (tok->kind != TK_EOF)
        error_tok(tok, "extra token"); // 说明有不合法的 token
    return node;
}
