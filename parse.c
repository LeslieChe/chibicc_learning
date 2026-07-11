#include "csub.h"

// All local variable instances created during parsing are
// accumulated to this list.
obj_t *g_locals;


static node_t *expr_stmt(token_t **rest, token_t *tok);
static node_t *expr(token_t **rest, token_t *tok);
static node_t *assign(token_t **rest, token_t *tok);
static node_t *equality(token_t **rest, token_t *tok);
static node_t *relational(token_t **rest, token_t *tok);
static node_t *add(token_t **rest, token_t *tok);
static node_t *mul(token_t **rest, token_t *tok);
static node_t *unary(token_t **rest, token_t *tok);
static node_t *primary(token_t **rest, token_t *tok);

// Find a local variable by name.
static obj_t *find_var(token_t *tok) {
  for (obj_t *var = g_locals; var; var = var->next)
    if (strlen(var->name) == tok->len && !strncmp(tok->loc, var->name, tok->len))
      return var;
  return NULL;
}

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

static node_t *new_var_node(obj_t *var)
{
    node_t *node = new_node(ND_VAR);
    node->var = var;
    return node;
}


static obj_t *new_lvar(char *name) {
  obj_t *var = calloc(1, sizeof(obj_t));
  var->name = name;
  var->next = g_locals; // insert to the head of the list
  g_locals = var;
  return var;
}


// stmt := expr-stmt
//       | "return" expr ";"
// 表达式语句 | return 语句
static node_t *stmt(token_t **rest, token_t *tok) {
  if (equal(tok, "return")) {
    node_t *node = new_unary(ND_RETURN, expr(&tok, tok->next));
    *rest = match_skip(tok, ";");
    return node;
  }

  return expr_stmt(rest, tok);
}


// expr-stmt := expr ";"
static node_t *expr_stmt(token_t **rest, token_t *tok)
{
    node_t *node = new_unary(ND_EXPR_STMT, expr(rest, tok));
    *rest = match_skip(*rest, ";");
    return node;
}

// expr := assign  赋值表达式
static node_t *expr(token_t **rest, token_t *tok)
{
    return assign(rest, tok);
}

// assign := equality ("=" assign)?
static node_t *assign(token_t **rest, token_t *tok)
{
    node_t *node = equality(&tok, tok);
    if (equal(tok, "="))
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next));
    *rest = tok;
    return node;
}

// assign := equality ("=" equality)*
// 上面的产生式是错误的

/*
下面的写法不对
赋值表达式的结合性是从右到左

static node_t *assign(token_t **rest, token_t *tok) {
  node_t *node = equality(&tok, tok);
  for(;;){
    if (equal(tok, "=")) {
        node = new_binary(ND_ASSIGN, node, equality(&tok, tok->next));
        continue;
    }

    *rest = tok;
    return node;
  }
}
*/



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
        return unary(rest, tok->next);  // 递归

    if (equal(tok, "-"))
        return new_unary(ND_NEG, unary(rest, tok->next));  // 递归

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

    if (tok->kind == TK_IDENT) {
        obj_t *var = find_var(tok);
        if (!var)
            var = new_lvar(strndup(tok->loc, tok->len));
        *rest = tok->next;
        return new_var_node(var);
    }

    if (tok->kind == TK_NUM) {
        node_t *node = new_num(tok->val);
        *rest = tok->next;
        return node;
    }

    error_tok(tok, "expected an expression");
}

// program := stmt*
// 程序 := 0 个或者多个 stmt

function_t *parse(token_t *tok)
{
    node_t head = {};
    node_t *cur = &head;

    while (tok->kind != TK_EOF) {
        cur = cur->next = stmt(&tok, tok);
    }

    function_t *prog = calloc(1, sizeof(function_t));
    prog->body = head.next;
    prog->locals = g_locals;
    return prog;
}