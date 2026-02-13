#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Forward declarations */

static void advance(Parser *p);
static Expr *parse_expression(Parser *p);
static Expr *parse_or(Parser *p);
static Expr *parse_and(Parser *p);
static Expr *parse_equality(Parser *p);
static Expr *parse_comparison(Parser *p);
static Expr *parse_term(Parser *p);
static Expr *parse_factor(Parser *p);
static Expr *parse_unary(Parser *p);
static Expr *parse_primary(Parser *p);

static Stmt *parse_statement(Parser *p);
static Stmt *parse_if(Parser *p);
static Stmt *parse_assignment(Parser *p);
static Stmt *parse_expr_statement(Parser *p);

static Stmt *parse_do(Parser *p);
static Stmt *parse_fn_def(Parser *p);
static Stmt *parse_return(Parser *p);

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
    return parse_or(p);
}

static Expr *parse_or(Parser *p) {
    Expr *expr = parse_and(p);

    while (p->current.type == TOK_OR) {
        advance(p);
        Expr *right = parse_and(p);

        Expr *binary = new_expr(EXPR_BINARY);
        binary->as.binary.op = BIN_OR;
        binary->as.binary.lhs = expr;
        binary->as.binary.rhs = right;

        expr = binary;
    }

    return expr;
}

static Expr *parse_and(Parser *p) {
    Expr *expr = parse_equality(p);

    while (p->current.type == TOK_AND) {
        advance(p);
        Expr *right = parse_equality(p);

        Expr *binary = new_expr(EXPR_BINARY);
        binary->as.binary.op = BIN_AND;
        binary->as.binary.lhs = expr;
        binary->as.binary.rhs = right;

        expr = binary;
    }

    return expr;
}

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

    if (match(p, TOK_NOT)) {
        Expr *right = parse_unary(p);
        Expr *unary = new_expr(EXPR_UNARY);
        unary->as.unary.op = UNOP_NOT;
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

    if (match(p, TOK_TRUE)) {
        Expr *expr = new_expr(EXPR_BOOL);
        expr->as.bool_val = 1;
        return expr;
    }

    if (match(p, TOK_FALSE)) {
        Expr *expr = new_expr(EXPR_BOOL);
        expr->as.bool_val = 0;
        return expr;
    }

    if (match(p, TOK_IDENT)) {
        Expr *expr = new_expr(EXPR_VAR);
        expr->as.var.name = strndup(p->previous.start,
                                    p->previous.length);
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


static Stmt *parse_do(Parser *p) {
    printf("parse_do: not implemented yet\n");
    exit(1);
}

static Stmt *parse_fn_def(Parser *p) {
    printf("parse_fn_def: not implemented yet\n");
    exit(1);
}

static Stmt *parse_return(Parser *p) {
    printf("parse_return: not implemented yet\n");
    exit(1);
}

/* =========================
   Statement parsing
   ========================= */

static Stmt *new_stmt(StmtKind kind) {
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = kind;
    return s;
}

static Stmt *parse_if(Parser *p) {
    advance(p);  // consume 'if'

    Expr *cond = parse_expression(p);

    if (p->current.type != TOK_NEWLINE) {
        printf("Expected newline after if condition\n");
        exit(1);
    }
    advance(p);  // consume newline

    Stmt **then_body = NULL;
    size_t then_count = 0;

    while (p->current.type != TOK_ELSE &&
           p->current.type != TOK_END &&
           p->current.type != TOK_EOF) {

        Stmt *stmt = parse_statement(p);

        then_body = realloc(then_body,
                            sizeof(Stmt*) * (then_count + 1));
        then_body[then_count++] = stmt;

        while (p->current.type == TOK_NEWLINE)
            advance(p);
    }

    Stmt **else_body = NULL;
    size_t else_count = 0;

    if (p->current.type == TOK_ELSE) {
        advance(p);  // consume 'else'

        if (p->current.type != TOK_NEWLINE) {
            printf("Expected newline after else\n");
            exit(1);
        }
        advance(p);  // consume newline

        while (p->current.type != TOK_END &&
               p->current.type != TOK_EOF) {

            Stmt *stmt = parse_statement(p);

            else_body = realloc(else_body,
                                sizeof(Stmt*) * (else_count + 1));
            else_body[else_count++] = stmt;

            while (p->current.type == TOK_NEWLINE)
                advance(p);
        }
    }

    if (p->current.type != TOK_END) {
        printf("Expected 'end' to close if\n");
        exit(1);
    }

    advance(p);  // consume 'end'

    Stmt *stmt = new_stmt(STMT_IF);
    stmt->as.if_stmt.cond = cond;
    stmt->as.if_stmt.then_body = then_body;
    stmt->as.if_stmt.then_count = then_count;
    stmt->as.if_stmt.else_body = else_body;
    stmt->as.if_stmt.else_count = else_count;

    return stmt;
}

static Stmt *parse_assignment(Parser *p) {
    Token ident = p->current;

    Lexer saved = *(p->lexer);
    Token next = lexer_next(&saved);

    if (next.type != TOK_ASSIGN)
        return NULL;

    advance(p);
    advance(p);

    Expr *value = parse_expression(p);

    Stmt *stmt = new_stmt(STMT_ASSIGN);
    stmt->as.assign.name = strndup(ident.start,
                                   ident.length);
    stmt->as.assign.value = value;

    return stmt;
}

static Stmt *parse_expr_statement(Parser *p) {
    Expr *expr = parse_expression(p);

    Stmt *stmt = new_stmt(STMT_EXPR);
    stmt->as.expr.expr = expr;

    return stmt;
}

static Stmt *parse_statement(Parser *p) {
    // pending statements to implement
    switch (p->current.type) {
        case TOK_IF:     return parse_if(p);
        case TOK_DO:     return parse_do(p);
        case TOK_FN:     return parse_fn_def(p);
        case TOK_RETURN: return parse_return(p);
        default: break;
    }

    if (p->current.type == TOK_IF)
        return parse_if(p);

    if (p->current.type == TOK_IDENT) {
        Stmt *assign = parse_assignment(p);
        if (assign)
            return assign;
    }

    return parse_expr_statement(p);
}

/* =========================
   Program
   ========================= */

Program *parse_program(Parser *p) {
    Program *program = malloc(sizeof(Program));
    program->stmts = NULL;
    program->count = 0;

    while (p->current.type != TOK_EOF) {

        while (p->current.type == TOK_NEWLINE)
            advance(p);

        if (p->current.type == TOK_EOF)
            break;

        Stmt *stmt = parse_statement(p);

        program->stmts = realloc(program->stmts,
            sizeof(Stmt*) * (program->count + 1));

        program->stmts[program->count++] = stmt;

        while (p->current.type == TOK_NEWLINE)
            advance(p);
    }

    return program;
}

Expr *parser_parse_expression(Parser *p) {
    return parse_expression(p);
}
