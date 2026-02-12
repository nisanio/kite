#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Forward declarations */

static void advance(Parser *p);
static Expr *parse_expression(Parser *p);
static Expr *parse_term(Parser *p);
static Expr *parse_factor(Parser *p);
static Expr *parse_unary(Parser *p);
static Expr *parse_primary(Parser *p);

/* =========================
   Utilities
   ========================= */

static void advance(Parser *p) {
    p->previous = p->current;
    p->current = lexer_next(p->lexer);
}

static int match(Parser *p, TokenType type) {
    if (p->current.type == type) {
        advance(p);
        return 1;
    }
    return 0;
}

/* =========================
   Init
   ========================= */

void parser_init(Parser *p, Lexer *lexer) {
    p->lexer = lexer;
    advance(p);
}

/* =========================
   Expression parsing
   ========================= */

static Expr *new_expr(ExprKind kind) {
    Expr *e = malloc(sizeof(Expr));
    e->kind = kind;
    return e;
}

static Expr *parse_expression(Parser *p) {
    return parse_term(p);
}

static Expr *parse_term(Parser *p) {
    Expr *expr = parse_factor(p);

    while (p->current.type == TOK_PLUS ||
           p->current.type == TOK_MINUS) {

        TokenType op = p->current.type;
        advance(p);

        Expr *right = parse_factor(p);

        Expr *binary = new_expr(EXPR_BINARY);
        binary->as.binary.op = (op == TOK_PLUS) ? BIN_ADD : BIN_SUB;
        binary->as.binary.lhs = expr;
        binary->as.binary.rhs = right;

        expr = binary;
    }

    return expr;
}

static Expr *parse_factor(Parser *p) {
    Expr *expr = parse_unary(p);

    while (p->current.type == TOK_STAR ||
           p->current.type == TOK_SLASH) {

        TokenType op = p->current.type;
        advance(p);

        Expr *right = parse_unary(p);

        Expr *binary = new_expr(EXPR_BINARY);
        binary->as.binary.op = (op == TOK_STAR) ? BIN_MUL : BIN_DIV;
        binary->as.binary.lhs = expr;
        binary->as.binary.rhs = right;

        expr = binary;
    }

    return expr;
}

static Expr *parse_unary(Parser *p) {
    if (match(p, TOK_MINUS)) {
        Expr *right = parse_unary(p);

        Expr *unary = new_expr(EXPR_UNARY);
        unary->as.unary.op = UNOP_NEG;
        unary->as.unary.rhs = right;

        return unary;
    }

    return parse_primary(p);
}

static Expr *parse_primary(Parser *p) {
    if (match(p, TOK_INT)) {
        Expr *expr = new_expr(EXPR_INT);

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.*s",
                 (int)p->previous.length,
                 p->previous.start);

        expr->as.int_val = atoll(buffer);
        return expr;
    }

    if (match(p, TOK_IDENT)) {
        Expr *expr = new_expr(EXPR_VAR);

        expr->as.var.name = strndup(
            p->previous.start,
            p->previous.length
        );

        return expr;
    }

    if (match(p, TOK_LPAREN)) {
        Expr *expr = parse_expression(p);
        if (!match(p, TOK_RPAREN)) {
            printf("Expected ')'\n");
            exit(1);
        }
        return expr;
    }

    printf("Unexpected token\n");
    exit(1);
}

/* =========================
   Program
   ========================= */

Program *parse_program(Parser *p) {
    (void)p;

    Program *program = malloc(sizeof(Program));
    program->stmts = NULL;
    program->count = 0;
    return program;
}

Expr *parser_parse_expression(Parser *p) {
    return parse_expression(p);
}

/* =========================
   Debug: print expression
   ========================= */

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void print_expr(Expr *expr, int indent) {
    if (!expr) return;

    print_indent(indent);

    switch (expr->kind) {
        case EXPR_INT:
            printf("INT(%lld)\n", (long long)expr->as.int_val);
            break;

        case EXPR_VAR:
            printf("VAR(%s)\n", expr->as.var.name);
            break;

        case EXPR_UNARY:
            printf("UNARY(-)\n");
            print_expr(expr->as.unary.rhs, indent + 1);
            break;

        case EXPR_BINARY:
            switch (expr->as.binary.op) {
                case BIN_ADD: printf("BINARY(+)\n"); break;
                case BIN_SUB: printf("BINARY(-)\n"); break;
                case BIN_MUL: printf("BINARY(*)\n"); break;
                case BIN_DIV: printf("BINARY(/)\n"); break;
                default: printf("BINARY(?)\n"); break;
            }
            print_expr(expr->as.binary.lhs, indent + 1);
            print_expr(expr->as.binary.rhs, indent + 1);
            break;

        default:
            printf("UNKNOWN_EXPR\n");
            break;
    }
}
