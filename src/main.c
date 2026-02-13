#include "lexer.h"
#include "parser.h"
#include "interp.h"
#include "env.h"
#include <stdio.h>

int main(void) {
    const char *source = "and or not";

    Lexer lexer;
    lexer_init(&lexer, source);

    Token tok;
    do {
        tok = lexer_next(&lexer);
        printf("Token type: %d\n", tok.type);
    } while (tok.type != TOK_EOF);

    return 0;
}
