#include "lexer.h"
#include <ctype.h>
#include <string.h>

/* =========================
   Helpers
   ========================= */

static int is_at_end(Lexer *l) {
    return *l->current == '\0';
}

static char advance(Lexer *l) {
    char c = *l->current++;
    l->col++;
    return c;
}

static char peek(Lexer *l) {
    return *l->current;
}

// Probably we need this in the future
/* static char peek_next(Lexer *l) {
    if (is_at_end(l)) return '\0';
    return l->current[1];
} */

static void skip_whitespace(Lexer *l) {
    for (;;) {
        char c = peek(l);

        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(l);
                break;

            // case '\n':
            //     advance(l);
            //     l->line++;
            //     l->col = 1;
            //     break;

            case '#':
                while (peek(l) != '\n' && !is_at_end(l)) {
                    advance(l);
                }
                break;

            default:
                return;
        }
    }
}

static Token make_token(Lexer *l, TokenType type, const char *start) {
    Token t;
    t.type = type;
    t.start = start;
    t.length = (size_t)(l->current - start);
    t.line = l->line;
    t.col = l->col;
    return t;
}

static Token error_token(Lexer *l, const char *start) {
    return make_token(l, TOK_ERROR, start);
}

/* =========================
   Identifiers & keywords
   ========================= */

static TokenType keyword_type(const char *start, size_t len) {
    if (len == 2 && strncmp(start, "if", 2) == 0) return TOK_IF;
    if (len == 4 && strncmp(start, "else", 4) == 0) return TOK_ELSE;
    if (len == 2 && strncmp(start, "do", 2) == 0) return TOK_DO;
    if (len == 3 && strncmp(start, "end", 3) == 0) return TOK_END;
    if (len == 2 && strncmp(start, "fn", 2) == 0) return TOK_FN;
    if (len == 6 && strncmp(start, "return", 6) == 0) return TOK_RETURN;
    if (len == 3 && strncmp(start, "and", 3) == 0) return TOK_AND;
    if (len == 2 && strncmp(start, "or", 2) == 0) return TOK_OR;
    if (len == 3 && strncmp(start, "not", 3) == 0) return TOK_NOT;
    if (len == 4 && strncmp(start, "true", 4) == 0) return TOK_TRUE;
    if (len == 5 && strncmp(start, "false", 5) == 0) return TOK_FALSE;
    if (len == 5 && strncmp(start, "until", 5) == 0) return TOK_UNTIL;

    return TOK_IDENT;
}

/* =========================
   Public API
   ========================= */

void lexer_init(Lexer *l, const char *source) {
    l->source = source;
    l->current = source;
    l->line = 1;
    l->col = 1;
}

Token lexer_next(Lexer *l) {
    skip_whitespace(l);

    const char *start = l->current;

    if (is_at_end(l)) {
        return make_token(l, TOK_EOF, start);
    }

    char c = advance(l);

    if (c == '\n') {
        l->line++;
        l->col = 1;
        return make_token(l, TOK_NEWLINE, start);
    }
    /* Numbers */
    if (isdigit(c)) {
        while (isdigit(peek(l))) advance(l);
        return make_token(l, TOK_INT, start);
    }

    /* Identifiers */
    if (isalpha(c) || c == '_') {
        while (isalnum(peek(l)) || peek(l) == '_') advance(l);
        Token t = make_token(l, TOK_IDENT, start);
        t.type = keyword_type(start, t.length);
        return t;
    }

    /* Strings */
    if (c == '"') {
        while (peek(l) != '"' && !is_at_end(l)) {
            if (peek(l) == '\n') {
                l->line++;
                l->col = 1;
            }
            advance(l);
        }

        if (is_at_end(l)) {
            return error_token(l, start);
        }

        advance(l); // closing "
        return make_token(l, TOK_STRING, start);
    }

    /* Operators and symbols */
    switch (c) {
        case '=':
            if (peek(l) == '=') {
                advance(l);
                return make_token(l, TOK_EQEQ, start);
            }
            return make_token(l, TOK_ASSIGN, start);

        case '!':
            if (peek(l) == '=') {
                advance(l);
                return make_token(l, TOK_NEQ, start);
            }
            return error_token(l, start);

        case '<':
            if (peek(l) == '=') {
                advance(l);
                return make_token(l, TOK_LTE, start);
            }
            return make_token(l, TOK_LT, start);

        case '>':
            if (peek(l) == '=') {
                advance(l);
                return make_token(l, TOK_GTE, start);
            }
            return make_token(l, TOK_GT, start);

        case '+': return make_token(l, TOK_PLUS, start);
        case '-': return make_token(l, TOK_MINUS, start);
        case '*': return make_token(l, TOK_STAR, start);
        case '/': return make_token(l, TOK_SLASH, start);

        case '(': return make_token(l, TOK_LPAREN, start);
        case ')': return make_token(l, TOK_RPAREN, start);
        case '[': return make_token(l, TOK_LBRACK, start);
        case ']': return make_token(l, TOK_RBRACK, start);
        case ',': return make_token(l, TOK_COMMA, start);
    }

    return error_token(l, start);
}
