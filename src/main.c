#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "interp.h"
#include "env.h"

int main(void) {
    const char *source =
        "x = 10\n"
        "x + 2\n";

    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    Program *program = parse_program(&parser);

    Env *env = env_create(NULL);

    eval_program(program, env);

    return 0;
}
