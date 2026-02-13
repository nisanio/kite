#include "lexer.h"
#include "parser.h"
#include "interp.h"
#include "env.h"
#include <stdio.h>

int main(void) {
    const char *source =
        "1 + 2 < 5\n"
        "1 + 2 == 4\n";

    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    Program *program = parse_program(&parser);

    Env *global = env_create(NULL);

    eval_program(program, global);

    return 0;
}
