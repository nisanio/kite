#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "interp.h"
#include "env.h"

int main(void) {
    // const char *source = "1 + 2 < 5";
    // const char *source = "1 + 2 == 3";
    const char *source = "1 < 2 == 1 < 3";

    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    Expr *expr = parser_parse_expression(&parser);

    print_expr(expr, 0);

    return 0;
}
