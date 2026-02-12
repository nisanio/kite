#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

static const char *token_name(TokenType type) {
    switch (type) {
        case TOK_INT: return "INT";
        case TOK_STRING: return "STRING";
        case TOK_IDENT: return "IDENT";

        case TOK_IF: return "IF";
        case TOK_ELSE: return "ELSE";
        case TOK_DO: return "DO";
        case TOK_END: return "END";
        case TOK_FN: return "FN";
        case TOK_RETURN: return "RETURN";
        case TOK_AND: return "AND";
        case TOK_OR: return "OR";
        case TOK_NOT: return "NOT";

        case TOK_ASSIGN: return "=";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_EQEQ: return "==";
        case TOK_NEQ: return "!=";
        case TOK_LT: return "<";
        case TOK_LTE: return "<=";
        case TOK_GT: return ">";
        case TOK_GTE: return ">=";

        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACK: return "[";
        case TOK_RBRACK: return "]";
        case TOK_COMMA: return ",";

        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

int main(void) {
    const char *source =
        "x = 10\n"
        "y = 20\n"
        "print(x + y)\n";

    Lexer lexer;
    lexer_init(&lexer, source);

    for (;;) {
        Token t = lexer_next(&lexer);

        printf("%-10s | %.*s\n",
               token_name(t.type),
               (int)t.length,
               t.start);

        if (t.type == TOK_EOF) break;
        if (t.type == TOK_ERROR) break;
    }

    return 0;
}
