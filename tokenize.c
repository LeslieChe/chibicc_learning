#include "csub.h"

/*
    Tokenizer (tokenize.c)
    ----------------------

    This file implements a tokenizer that converts a string of C code
    into a linked list of tokens. Each token represents a meaningful
    unit in the code, such as keywords, identifiers, literals, and
    operators.

    The tokenizer recognizes the following types of tokens:
    - TK_PUNCT: Keywords or punctuators (e.g., operators, braces)
    - TK_NUM: Numeric literals (e.g., integers)
    - TK_EOF: End-of-file markers

    The tokenizer processes the input string character by character,
    skipping whitespace and identifying tokens based on their
    characteristics.

*/

// Input string
static char *current_input;

// Reports an error and exit.
void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Reports an error location and exit.
static void verror_at(char *loc, char *fmt, va_list ap)
{
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);  // 打印当前输入的字符串

    /*
          %*s 中的 * 表示宽度由后面的 pos 参数提供，
          按该宽度输出后面的字符串；这里给的是空字符串 ""，
          所以实际效果是输出 pos 个空格。
    */
    fprintf(stderr, "%*s", pos, "");  // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(token_t *tok, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

// Consumes the current token_t if it matches `op`.
bool equal(token_t *tok, char *op)
{
    return tok->len == (int)strlen(op) &&
           memcmp(tok->loc, op, tok->len) == 0;
}

// Ensure that the current token_t is `op`.
token_t *match_skip(token_t *tok, char *op)
{
    if (!equal(tok, op))
        error_tok(tok, "expected '%s'", op);
    return tok->next;
}

// Create a new token.
static token_t *new_token(token_kind_e kind, char *start, char *end)
{
    token_t *tok = calloc(1, sizeof(token_t));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    return tok;
}

// p starts with q
static bool startswith(char *p, char *q)
{
    return strncmp(p, q, strlen(q)) == 0;
}

// Read a punctuator token from p and returns its length.
static int read_punct(char *p)
{
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">="))
        return 2;

    // 这里已经包含了贪心法则
    
    // 在默认 C
    // 语言环境里通常包含所有可打印的、不是字母也不是数字的标点字符。
    return ispunct(*p) ? 1 : 0;
}

// Tokenize `current_input` and returns new tokens.
token_t *tokenize(char *p)
{
    current_input = p;
    token_t head = {};
    token_t *cur = &head;

    while (*p) {
        // Skip whitespace characters.
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Numeric literal
        if (isdigit(*p)) {
            cur = cur->next = new_token(TK_NUM, p, p);
            char *q = p;
            /*
              strtoul()
              会将字符串转换为无符号长整型数值，
              并返回转换后第一个不符合数字的字符的指针。
              例如，strtoul("123abc", &p, 10) 会将 p 指向 'a'，并返回
              123。
            */
            cur->val = strtoul(p, &p, 10);

            /*
              p points to the first character after the number,
              so the length is p - q
            */
            cur->len = p - q;
            continue;
        }

        // Punctuators
        int punct_len = read_punct(p);
        if (punct_len) {
            cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
            p += cur->len;
            continue;
        }

        error_at(p, "invalid token");
    }

    cur = cur->next = new_token(TK_EOF, p, p);
    return head.next;
}
