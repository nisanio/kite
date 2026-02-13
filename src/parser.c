#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Forward declarations */

static void advance(Parser *p);
static Expr *parse_expression(Parser *p);
static Expr *parse_equality(Parser *p);
static Expr *parse_comparison(Parser *p);
static Expr *parse_term(Parser *p);
static Expr *parse_factor(Parser *p);
static Expr *parse_unary(Parser *p);
static Expr *parse_primary(Parser *p);
static Stmt *parse_statement(Parser *p);

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
    return parse_equality(p);
}

/* equality: == != */
static Expr *parse_equality(Parser *p) {
    Expr *expr = parse_comparison(p);

    while (p->current.type == TOK_EQEQ ||
           p->current.type == TOK_NEQ) {

        TokenType op = p->current.type;
        advance(p);

        Expr *right = parse_comparison(p);

        Expr *binary = new_expr(EXPR_BINARY);
        binary->as.binary.op = (op == TOK_EQEQ) ? BIN_EQ : BIN_NEQ;
        binary->as.binary.lhs = expr;
        binary->as.binary.rhs = right;

        expr = binary;
    }

    return expr;
}

/* comparison: < <= > >= */
static Expr *parse_comparison(Parser *p) {
    Expr *expr = parse_term(p);

    while (p->current.type == TOK_LT  ||
           p->current.type == TOK_LTE ||
           p->current.type == TOK_GT  ||
           p->current.type == TOK_GTE) {

        TokenType op = p->current.type;
        advance(p);

        Expr *right = parse_term(p);

        Expr *binary = new_expr(EXPR_BINARY);

        switch (op) {
            case TOK_LT:  binary->as.binary.op = BIN_LT; break;
            case TOK_LTE: binary->as.binary.op = BIN_LTE; break;
            case TOK_GT:  binary->as.binary.op = BIN_GT; break;
            case TOK_GTE: binary->as.binary.op = BIN_GTE; break;
            default: break;
        }

        binary->as.binary.lhs = expr;
        binary->as.binary.rhs = right;

        expr = binary;
    }

    return expr;
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

static Stmt *new_stmt(StmtKind kind) {
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = kind;
    return s;
}

static Stmt *parse_statement(Parser *p) {

    if (p->current.type == TOK_IDENT) {
        Token ident = p->current;

        Lexer saved = *(p->lexer);
        Token next = lexer_next(&saved);

        if (next.type == TOK_ASSIGN) {
            advance(p);
            advance(p);

            Expr *value = parse_expression(p);

            Stmt *stmt = new_stmt(STMT_ASSIGN);
            stmt->as.assign.name = strndup(ident.start, ident.length);
            stmt->as.assign.value = value;

            return stmt;
        }
    }

    Expr *expr = parse_expression(p);

    Stmt *stmt = new_stmt(STMT_EXPR);
    stmt->as.expr.expr = expr;

    return stmt;
}

/* =========================
   Program
   ========================= */
Program *parse_program(Parser *p) {
    Program *program = malloc(sizeof(Program));
    program->stmts = NULL;
    program->count = 0;

    while (p->current.type != TOK_EOF) {

        while (p->current.type == TOK_NEWLINE) {
            advance(p);
        }

        if (p->current.type == TOK_EOF) {
            break;
        }

        Stmt *stmt = parse_statement(p);

        program->stmts = realloc(
            program->stmts,
            sizeof(Stmt*) * (program->count + 1)
        );

        program->stmts[program->count++] = stmt;

        while (p->current.type == TOK_NEWLINE) {
            advance(p);
        }
    }

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
                case BIN_EQ:  printf("BINARY(==)\n"); break;
                case BIN_NEQ: printf("BINARY(!=)\n"); break;
                case BIN_LT:  printf("BINARY(<)\n"); break;
                case BIN_LTE: printf("BINARY(<=)\n"); break;
                case BIN_GT:  printf("BINARY(>)\n"); break;
                case BIN_GTE: printf("BINARY(>=)\n"); break;
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
