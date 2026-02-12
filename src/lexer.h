#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

/* =========================
   Token types
   ========================= */

typedef enum {
    TOK_INT,
    TOK_STRING,
    TOK_IDENT,

    /* Keywords */
    TOK_IF,
    TOK_ELSE,
    TOK_DO,
    TOK_END,
    TOK_FN,
    TOK_RETURN,
    TOK_AND,
    TOK_OR,
    TOK_NOT,

    /* Operators */
    TOK_ASSIGN,     // =
    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_STAR,       // *
    TOK_SLASH,      // /
    TOK_EQEQ,       // ==
    TOK_NEQ,        // !=
    TOK_LT,         // <
    TOK_LTE,        // <=
    TOK_GT,         // >
    TOK_GTE,        // >=

    /* Symbols */
    TOK_LPAREN,     // (
    TOK_RPAREN,     // )
    TOK_LBRACK,     // [
    TOK_RBRACK,     // ]
    TOK_COMMA,      // ,

    TOK_EOF,
    TOK_ERROR
} TokenType;

/* =========================
   Token structure
   ========================= */

typedef struct {
    TokenType type;

    const char *start;   // pointer into source
    size_t length;

    int line;
    int col;
} Token;

/* =========================
   Lexer structure
   ========================= */

typedef struct {
    const char *source;
    const char *current;

    int line;
    int col;
} Lexer;

/* =========================
   API
   ========================= */

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next(Lexer *lexer);

#endif
