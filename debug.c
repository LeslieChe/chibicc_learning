#include "csub.h"

#include <unistd.h>

void print_node_recursive(node_t *node, int depth, bool is_last,
                          char *label);
const char *get_ast_node_kind_name(node_kind_e kind);
static bool is_unary_op(node_kind_e kind);
static bool is_binary_op(node_kind_e kind);

void print_function(function_t *fn) {
    printf("Function: %s\n", fn->name);
    printf("Stack Size: %d\n", fn->stack_size);
    printf("Parameters:\n");
    for (obj_t *var = fn->params; var; var = var->next) {
        printf("  Arg: %s", var->name);
        printf("  Type: ");
        print_type(var->ty);
        printf("  Offset: %d\n", var->offset);
    }

}

// 辅助：获取节点类型字符串
const char *get_ast_node_kind_name(node_kind_e kind)
{
    switch (kind) {
        case ND_ADD:
            return "+";
        case ND_SUB:
            return "-";
        case ND_MUL:
            return "mul";
        case ND_DIV:
            return "/";
        case ND_NEG:
            return "neg";
        case ND_EQ:
            return "==";
        case ND_NE:
            return "!=";
        case ND_LT:
            return "<";
        case ND_LE:
            return "<=";
        case ND_ASSIGN:
            return "=";
        case ND_ADDR:
            return "&";
        case ND_DEREF:
            return "*";
        case ND_RETURN:
            return "return";
        case ND_IF:
            return "if";
        case ND_FOR:
            return "while or for";
        case ND_BLOCK:
            return "block";
        case ND_EXPR_STMT:  // Expression statement
            return "EXPR_STMT";
        case ND_VAR:  // Variable
            return "VAR";
        case ND_FUNCALL:
            return "FUNCALL";
        case ND_NUM:
            return "NUM";
        default:
            return "UNKNOWN";
    }
}
// 辅助函数：判断是否为一元运算符
static bool is_unary_op(node_kind_e kind)
{
    return kind == ND_NEG || kind == ND_ADDR || kind == ND_DEREF;
}

// 辅助函数：判断是否为二元运算符
static bool is_binary_op(node_kind_e kind)
{
    return kind == ND_ADD || kind == ND_SUB || kind == ND_MUL ||
           kind == ND_DIV || kind == ND_EQ || kind == ND_NE ||
           kind == ND_LT || kind == ND_LE || kind == ND_ASSIGN;
}

// 全局路径追踪数组，用于判断每一层缩进是否需要画垂直线 │
static bool line_mask[128];

// --- 辅助函数：打印缩进和树形符号 ---
void print_indent(int depth, bool is_last)
{
    for (int i = 0; i < depth; i++) {
        printf(line_mask[i] ? "│   " : "    ");
    }
    printf(is_last ? "└── " : "├── ");
}

// --- 顶层入口：打印整个 Function 结构 ---
void dump_function(function_t *fn)
{
    if (!fn)
        return;

    printf("Stack Size: %d\n", fn->stack_size);
    printf("Locals:\n");
    for (obj_t *v = fn->locals; v; v = v->next) {
        printf("  - \033[1;36m%s\033[0m (offset: %3d)  ", v->name,
               v->offset);
        printf("\033[1;36m  ");
        print_type(v->ty);
        printf("\033[0m\n");
    }
    printf("\nAST Structure:\n");
    memset(line_mask, 0, sizeof(line_mask));
    print_node_recursive(fn->body, 0, true, "body:");
}

void dump_ast_tree(function_t *fn)
{
    if (!fn)
        return;

    memset(line_mask, 0, sizeof(line_mask));
    print_node_recursive(fn->body, 0, true, "body:");
}

void dump_ast_node(node_t *node, char *label)
{
    if (!node)
        return;

    memset(line_mask, 0, sizeof(line_mask));
    print_node_recursive(node, 0, true, label);
}

void print_node_recursive(node_t *node, int depth, bool is_last,
                          char *label)
{
    if (!node)
        return;

    // 1. 打印父级引导线 (根据之前的 line_mask 决定每一列是 │
    // 还是空格)
    for (int i = 0; i < depth; i++) {
        printf(line_mask[i] ? "│   " : "    ");
    }

    // 2. 打印当前层连线和成员标签

    printf(is_last ? "└── " : "├── ");
    if (label)
        printf("\033[1;33m%s \033[0m", label);  // 黄色标签

    // 3. 打印节点类型名 [KIND]
    printf("\033[1;37m[%s]\033[0m  ",
           get_ast_node_kind_name(node->kind));
    print_type(node->ty);  // 打印类型信息
    // __print_tok(node->tok);
      printf ("\n");
   // printf("   \033[1;36m%s\033[0m\n",  node->debug_info);
 
    // 4. 更新 line_mask 状态：如果当前不是最后一个，则下方需要垂直线

    line_mask[depth] = !is_last;

    // 5. 递归处理成员 (Depth + 1)
    int next_d = depth + 1;

    switch (node->kind) {
        case ND_NUM:
            print_indent(next_d, true);
            printf("\033[1;32mValue:  %d\033[0m\n", node->val);
            break;

        case ND_VAR:
            if (node->var) {
                // 第一个属性：Name
                print_indent(next_d, false);
                printf("\033[1;36mName:   %s\033[0m\n",
                       node->var->name);

                print_indent(next_d, false);
                printf("\033[1;36mType:   ");
                print_type(node->var->ty);
                printf("\033[0m\n");
                // 第二个属性：Offset
                print_indent(next_d, true);
                printf("\033[1;36mOffset: %d\033[0m\n",
                       node->var->offset);
            }
            break;

        case ND_FUNCALL:
            print_indent(next_d, true);
            printf("\033[1;35mFunction: %s\033[0m\n", node->funcname);
            break;

        case ND_RETURN:
            if (!node->lhs) {
                print_indent(next_d, true);
                printf("\033[1;30m(void return)\033[0m\n");
            }
            break;

        default:
            break;
    }

    switch (node->kind) {
        case ND_IF:
            print_node_recursive(node->cond, next_d, false, "cond:");
            print_node_recursive(node->then, next_d,
                                 node->els ? false : true, "then:");
            if (node->els)
                print_node_recursive(node->els, next_d, true, "els:");
            break;

        case ND_FOR:
            if (node->init)
                print_node_recursive(node->init, next_d, false,
                                     "init:");
            if (node->cond)
                print_node_recursive(node->cond, next_d, false,
                                     "cond:");
            if (node->inc)
                print_node_recursive(node->inc, next_d, false,
                                     "inc:");
            print_node_recursive(node->then, next_d, true, "then:");
            break;

        case ND_BLOCK:
            // 精确体现 body 和 next 的链表结构
            if (node->body) {
                node_t *nd = node->body;
                // 打印第一个节点作为 body
                print_node_recursive(
                    nd, next_d, nd->next ? false : true, "body:");

                // 打印后续节点作为 next
                for (nd = nd->next; nd; nd = nd->next) {
                    print_node_recursive(
                        nd, next_d, nd->next ? false : true, "next:");
                }
            }
            break;

        case ND_EXPR_STMT:
        case ND_RETURN:
            print_node_recursive(node->lhs, next_d, true, "lhs:");
            break;

        default:
            if (is_binary_op(node->kind)) {
                print_node_recursive(node->lhs, next_d, false,
                                     "lhs:");
                print_node_recursive(node->rhs, next_d, true, "rhs:");
            } else if (is_unary_op(node->kind)) {
                print_node_recursive(node->lhs, next_d, true, "lhs:");
            }
            break;
    }
}


void __print_tok(token_t *tok)
{
    if (!tok || !tok->loc) {
        printf(" (no token)");
        return;
    }
    printf("%.*s", tok->len, tok->loc);
}

void print_type(type_t *ty) {
    if (!ty) {
        printf("(null)");
        return;
    }

    switch (ty->kind) {
    case TY_INT:
        printf("int");
        break;
    
    case TY_PTR:
        printf("ptr to ");
        print_type(ty->base); // 递归打印指向的类型
        break;
    case TY_ARRAY:
        printf("[%d]", ty->array_len);
        printf(" ele type: ");
        print_type(ty->base);
        
        break;

    case TY_FUNC:
        printf("func:");       
        printf(" (");
        for (type_t *param = ty->params; param; param = param->next) {
            print_type(param);
            if (param->next)
                printf(", ");
        }
        printf(") ");
        printf("return: ");
        print_type(ty->return_ty);
        break;
 
    default:
        printf("unknown_type");
    }
}

static void trim_newline(char *line)
{
    line[strcspn(line, "\r\n")] = '\0';
}

static size_t strip_ansi(char *line)
{
    char *src = line;
    char *dst = line;
    while (*src) {
        if ((unsigned char)src[0] == 0x1b && src[1] == '[') {
            src += 2;
            while (*src && !((*src >= '@' && *src <= '~')))
                src++;
            if (*src)
                src++;
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    return (size_t)(dst - line);
}

static void print_expression_view(function_t *fn, node_t *node, char *label)
{
    FILE *assembly = tmpfile();
    FILE *ast = tmpfile();
    if (!assembly || !ast)
        error("cannot create temporary output");

    int saved_stdout = dup(fileno(stdout));
    if (saved_stdout < 0)
        error("cannot save standard output");

    fflush(stdout);
    dup2(fileno(assembly), fileno(stdout));
    codegen_node(fn, node);
    fflush(stdout);

    dup2(fileno(ast), fileno(stdout));
    dump_ast_node(node, label);
    fflush(stdout);

    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);

    size_t ast_count = 0;
    size_t ast_width = 0;
    char line[1024];
    rewind(ast);
    while (fgets(line, sizeof(line), ast)) {
        trim_newline(line);
        size_t width = strip_ansi(line);
        if (width > ast_width)
            ast_width = width;
        ast_count++;
    }

    rewind(ast);
    rewind(assembly);
    for (size_t row = 1; fgets(line, sizeof(line), ast); row++) {
        trim_newline(line);
        strip_ansi(line);

        char asm_line[1024] = "";
        const char *right = "";
        if (fgets(asm_line, sizeof(asm_line), assembly)) {
            trim_newline(asm_line);
            if (row == ast_count) {
                char extra[1024];
                right = fgets(extra, sizeof(extra), assembly) ? "..." : asm_line;
            } else {
                right = asm_line;
            }
        }
        printf("%-*s | %s\n", (int)ast_width, line, right);
    }

    fclose(ast);
    fclose(assembly);
}

static void print_statement_views(function_t *fn, node_t *node)
{
    if (!node)
        return;

    switch (node->kind) {
    case ND_BLOCK:
        for (node_t *child = node->body; child; child = child->next)
            print_statement_views(fn, child);
        return;
    case ND_EXPR_STMT:
    case ND_RETURN:
        print_expression_view(fn, node->lhs, "lhs:");
        printf("\n");
        return;
    case ND_IF:
        print_expression_view(fn, node->cond, "cond:");
        printf("\n");
        print_statement_views(fn, node->then);
        print_statement_views(fn, node->els);
        return;
    case ND_FOR:
        if (node->init)
            print_statement_views(fn, node->init);
        if (node->cond) {
            print_expression_view(fn, node->cond, "cond:");
            printf("\n");
        }
        if (node->inc) {
            print_expression_view(fn, node->inc, "inc:");
            printf("\n");
        }
        print_statement_views(fn, node->then);
        return;
    default:
        return;
    }
}

void dump_ast_asm(function_t *prog)
{
    for (function_t *fn = prog; fn; fn = fn->next)
        print_statement_views(fn, fn->body);
}
