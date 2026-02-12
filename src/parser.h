#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lexer;
    Token current;
    Token previous;
} Parser;

void parser_init(Parser *parser, Lexer *lexer);
Program *parse_program(Parser *parser);

void print_expr(Expr *expr, int indent);
Expr *parser_parse_expression(Parser *p);
#endif
