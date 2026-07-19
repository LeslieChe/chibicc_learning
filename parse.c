#include "csub.h"

// All local variable instances created during parsing are
// accumulated to this list.
obj_t *g_locals;

static node_t *compound_stmt(token_t **rest, token_t *tok);
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
static obj_t *find_var(token_t *tok)
{
    for (obj_t *var = g_locals; var; var = var->next)
        if (strlen(var->name) == tok->len &&
            !strncmp(tok->loc, var->name, tok->len))
            return var;
    return NULL;
}

static node_t *new_node(node_kind_e kind, token_t *tok)
{
    node_t *node = calloc(1, sizeof(node_t));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static node_t *new_binary(node_kind_e kind, node_t *lhs, node_t *rhs, token_t *tok)
{
    node_t *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    node->tok = tok;
    return node;
}

static node_t *new_unary(node_kind_e kind, node_t *expr, token_t *tok)
{
    node_t *node = new_node(kind, tok);

    // 一元仅有一个操作数，存放在 lhs 中
    node->lhs = expr;
    return node;
}

static node_t *new_num(int val, token_t *tok)
{
    node_t *node = new_node(ND_NUM, tok);

    // ND_NUM 节点的值存放在 val 中
    node->val = val;
    return node;
}

static node_t *new_var_node(obj_t *var, token_t *tok)
{
    node_t *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

static obj_t *new_lvar(char *name, type_t *ty)
{
    obj_t *var = calloc(1, sizeof(obj_t));
    var->name = name;
    var->ty = ty;
    var->next = g_locals;  // insert to the head of the list
    g_locals = var;
    return var;
}

static char *get_ident(token_t *tok) {
  if (tok->kind != TK_IDENT)
    error_tok(tok, "expected an identifier");
  return strndup(tok->loc, tok->len);
}


// declspec = "int"
static type_t *declspec(token_t **rest, token_t *tok) {
  *rest = match_skip(tok, "int");
  return ty_int;
}


// declarator = "*"* ident
static type_t *declarator(token_t **rest, token_t *tok, type_t *ty) {
  while (consume(&tok, tok, "*"))
    ty = pointer_to(ty);

  if (tok->kind != TK_IDENT)
    error_tok(tok, "expected a variable name");

  ty->name = tok; // 暂存名字
  *rest = tok->next;
  return ty;
}



// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
/*
    最外层是 declspec “;”
    declspec 目前只有一种类型：int
    标识符可以省略, 例如： int;
    declarator 可以是 0 到多个 *，然后是标识符
    ("=" expr) 是初始化式，可选
    如果一次声明多个变量，则用逗号分隔，所以有 
    ("," declarator ("=" expr)?)*
    
*/
static node_t *declaration(token_t **rest, token_t *tok) {
  type_t *base_ty = declspec(&tok, tok);

  node_t head = {};
  node_t *cur = &head;
  int i = 0;

  while (!equal(tok, ";")) {
    if (i++ > 0)
      tok = match_skip(tok, ",");

    type_t *ty = declarator(&tok, tok, base_ty);
    obj_t *var = new_lvar(get_ident(ty->name), ty);

    if (!equal(tok, "="))
      continue;

    node_t *lhs = new_var_node(var, ty->name);
    node_t *rhs = assign(&tok, tok->next);
    node_t *node = new_binary(ND_ASSIGN, lhs, rhs, tok);

    // 这里把初始化语句当作一个表达式语句来处理，放在 ND_EXPR_STMT 节点中
    cur = cur->next = new_unary(ND_EXPR_STMT, node, tok);
  }

  // 多个声明语句（逗号分隔）被包装在一个 ND_BLOCK 节点中
  node_t *node = new_node(ND_BLOCK, tok);
  node->body = head.next;
  *rest = tok->next;
  return node;
}

// stmt := expr-stmt
//       | "if" "(" expr ")" stmt ("else" stmt)?
//       | "for" "(" expr-stmt expr? ";" expr? ")" stmt
//       | "while" "(" expr ")" stmt
//       | "{" compound-stmt
//       | "return" expr ";"

static node_t *stmt(token_t **rest, token_t *tok)
{
    if (equal(tok, "return")) {
        node_t *node = new_node(ND_RETURN, tok);
        node->lhs = expr(&tok, tok->next);
        *rest = match_skip(tok, ";");
        return node;
    }

    if (equal(tok, "if")) {
        node_t *node = new_node(ND_IF, tok);
        tok = match_skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = match_skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else"))
            node->els = stmt(&tok, tok->next);
        *rest = tok;
        return node;
    }

    //    "for" "(" expr-stmt expr? ";" expr? ")" stmt
    if (equal(tok, "for")) {
        node_t *node = new_node(ND_FOR, tok);
        tok = match_skip(tok->next, "(");

        node->init = expr_stmt(&tok, tok);

        if (!equal(tok, ";"))
            node->cond = expr(&tok, tok);
        tok = match_skip(tok, ";");

        if (!equal(tok, ")"))
            node->inc = expr(&tok, tok);
        tok = match_skip(tok, ")");

        node->then = stmt(rest, tok);
        return node;
    }

    if (equal(tok, "while")) {
        node_t *node = new_node(ND_FOR, tok);
        tok = match_skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = match_skip(tok, ")");
        node->then = stmt(rest, tok);
        return node;
    }

    if (equal(tok, "{"))
        return compound_stmt(rest, tok->next);

    return expr_stmt(rest, tok);
}

// compound-stmt := (declaration | stmt)* "}"
static node_t *compound_stmt(token_t **rest, token_t *tok)
{
    node_t *node = new_node(ND_BLOCK, tok); // 我感觉第二个参数没有意义
    node_t head = {};
    node_t *cur = &head;
    while (!equal(tok, "}")) {
        if (equal(tok, "int")) {
            cur = cur->next = declaration(&tok, tok);
        } else {
            cur = cur->next = stmt(&tok, tok);
        }
        add_type(cur);
    }
    

    node->body = head.next;
    *rest = tok->next;
    return node;
}

// expr-stmt := expr? ";"
/*
    我的理解：
    C 语言里空语句 `;` 和空块 `{}` 语义接近：都代表 “什么都不执行”。
    编译器作者在这里做了一层统一：遇到裸分号，直接用空块节点表示，
    后续 IR / 代码生成、语义分析不用单独写一套「空语句」逻辑，
    复用空块的处理逻辑。
*/
static node_t *expr_stmt(token_t **rest, token_t *tok)
{
    if (equal(tok, ";")) {
        *rest = tok->next;
        return new_node(ND_BLOCK, tok);  // ; 等同于 {}
    }
    node_t *node = new_node(ND_EXPR_STMT, tok); // 第二个参数的意义何在？
    node->lhs = expr(&tok, tok);
    *rest = match_skip(tok, ";");
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
        return new_binary(ND_ASSIGN, node, assign(rest, tok->next), tok);
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
        token_t *start = tok;
        if (equal(tok, "==")) {
            node =
                new_binary(ND_EQ, node, relational(&tok, tok->next), tok);
            continue;
        }

        if (equal(tok, "!=")) {
            node =
                new_binary(ND_NE, node, relational(&tok, tok->next), tok);
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
        token_t *start = tok;
        if (equal(tok, "<")) {
            node = new_binary(ND_LT, node, add(&tok, tok->next), tok);
            continue;
        }

        if (equal(tok, "<=")) {
            node = new_binary(ND_LE, node, add(&tok, tok->next), tok);
            continue;
        }

        if (equal(tok, ">")) {
            node = new_binary(ND_LT, add(&tok, tok->next), node, tok);
            continue;
        }

        if (equal(tok, ">=")) {
            node = new_binary(ND_LE, add(&tok, tok->next), node, tok);
            continue;
        }

        *rest = tok;
        return node;
    }
}



static node_t *new_add(node_t *lhs, node_t *rhs, token_t *tok) {
  add_type(lhs);
  add_type(rhs);

  // num + num
  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_ADD, lhs, rhs, tok);

  if (lhs->ty->base && rhs->ty->base) // 两个指针相加
    error_tok(tok, "invalid operands");

  // Canonicalize `num + ptr` to `ptr + num`.
  if (!lhs->ty->base && rhs->ty->base) { // 交换 lhs 和 rhs
    node_t *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }

  // ptr + num
  rhs = new_binary(ND_MUL, rhs, new_num(8, tok), tok); // 这里认为指针指向的都是 8 字节的类型
  return new_binary(ND_ADD, lhs, rhs, tok);
}

// Like `+`, `-` is overloaded for the pointer type.
static node_t *new_sub(node_t *lhs, node_t *rhs, token_t *tok) {
  add_type(lhs);
  add_type(rhs);

  // num - num
  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_SUB, lhs, rhs, tok);

  // ptr - num
  if (lhs->ty->base && is_integer(rhs->ty)) {
    rhs = new_binary(ND_MUL, rhs, new_num(8, tok), tok);
    add_type(rhs);
    node_t *node = new_binary(ND_SUB, lhs, rhs, tok);
    node->ty = lhs->ty;
    return node;
  }

  // ptr - ptr, which returns how many elements are between the two.
  if (lhs->ty->base && rhs->ty->base) {
    node_t *node = new_binary(ND_SUB, lhs, rhs, tok);
    node->ty = ty_int;
    return new_binary(ND_DIV, node, new_num(8, tok), tok);
  }

  error_tok(tok, "invalid operands");
}


// add := mul ("+" mul | "-" mul)*
// mul 是乘性表达式
static node_t *add(token_t **rest, token_t *tok)
{
    node_t *node = mul(&tok, tok);

    for (;;) {
        token_t *start = tok;
        if (equal(tok, "+")) {
            node =  new_add(node, mul(&tok, tok->next), tok);
            continue;
        }

        if (equal(tok, "-")) {
            node = new_sub(node, mul(&tok, tok->next), tok);
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
        token_t *start = tok;
        if (equal(tok, "*")) {
            node = new_binary(ND_MUL, node, unary(&tok, tok->next), tok);
            continue;
        }

        if (equal(tok, "/")) {
            node = new_binary(ND_DIV, node, unary(&tok, tok->next), tok);
            continue;
        }

        *rest = tok;    
        return node;
    }
}

// unary = ("+" | "-" | "*" | "&") unary
//       | primary
static node_t *unary(token_t **rest, token_t *tok)
{
    if (equal(tok, "+"))
        return unary(rest, tok->next);  // 递归

    if (equal(tok, "-"))
        return new_unary(ND_NEG, unary(rest, tok->next),
                         tok);  // 递归

    if (equal(tok, "&"))
        return new_unary(ND_ADDR, unary(rest, tok->next), tok);

    if (equal(tok, "*"))
        return new_unary(ND_DEREF, unary(rest, tok->next), tok);
    return primary(rest, tok);
}

// primary := "(" expr ")" | ident args? | num
// args := "(" ")"
static node_t *primary(token_t **rest, token_t *tok)
{
    if (equal(tok, "(")) {
        node_t *node = expr(rest, tok->next);
        *rest = match_skip(*rest, ")");
        return node;
    }

    if (tok->kind == TK_IDENT) {
        // Function call
        if (equal(tok->next, "(")) {
            node_t *node = new_node(ND_FUNCALL, tok);
            node->funcname = strndup(tok->loc, tok->len);
            *rest = match_skip(tok->next->next, ")");
            return node;
        }

        // Variable
        obj_t *var = find_var(tok);
        if (!var)
            error_tok(tok, "undefined variable");
        *rest = tok->next;
        return new_var_node(var, tok);
    }

    if (tok->kind == TK_NUM) {
        node_t *node = new_num(tok->val, tok);
        *rest = tok->next;
        return node;
    }

    error_tok(tok, "expected an expression");
}

// program := { compound-stmt
function_t *parse(token_t *tok)
{
    tok = match_skip(tok, "{");
    function_t *prog = calloc(1, sizeof(function_t));
    prog->body = compound_stmt(&tok, tok);
    prog->locals = g_locals;
    return prog;
}