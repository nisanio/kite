#include "parser.h"
#include <stdlib.h>

/* Forward declarations */
static void advance(Parser *p);

/* =========================
   Init
   ========================= */

void parser_init(Parser *p, Lexer *lexer) {
    p->lexer = lexer;
    advance(p);
}

/* =========================
   Advance
   ========================= */

static void advance(Parser *p) {
    p->previous = p->current;
    p->current = lexer_next(p->lexer);
}

/* =========================
   Program
   ========================= */

Program *parse_program(Parser *p) {
    (void)p;

    /* Placeholder for now */
    Program *program = malloc(sizeof(Program));
    program->stmts = NULL;
    program->count = 0;
    return program;
}
