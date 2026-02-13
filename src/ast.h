#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>

/* Forward declarations */
typedef struct Expr Expr;
typedef struct Stmt Stmt;

/* =========================
   EXPRESSIONS
   ========================= */

typedef enum {
    EXPR_INT,
    EXPR_BOOL, 
    EXPR_STRING,
    EXPR_VAR,
    EXPR_ARRAY,
    EXPR_CALL,
    EXPR_INDEX,
    EXPR_UNARY,
    EXPR_BINARY
} ExprKind;

typedef enum {
    UNOP_NEG,
    UNOP_NOT
} UnOp;

typedef enum {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_EQ,
    BIN_NEQ,
    BIN_LT,
    BIN_LTE,
    BIN_GT,
    BIN_GTE,
    BIN_AND,
    BIN_OR
} BinOp;

struct Expr {
    ExprKind kind;
    int line;
    int col;

    union {
        int64_t int_val;
        int bool_val; 
        
        struct {
            char *data;
            size_t len;
        } string;

        struct {
            char *name;
        } var;

        struct {
            Expr **items;
            size_t count;
        } array;

        struct {
            char *callee;
            Expr **args;
            size_t argc;
        } call;

        struct {
            Expr *base;
            Expr *index;
        } index;

        struct {
            UnOp op;
            Expr *rhs;
        } unary;

        struct {
            BinOp op;
            Expr *lhs;
            Expr *rhs;
        } binary;

    } as;
};

/* =========================
   STATEMENTS
   ========================= */

typedef enum {
    STMT_ASSIGN,
    STMT_EXPR,
    STMT_IF,
    STMT_DO,
    STMT_FNDEF,
    STMT_RETURN
} StmtKind;

struct Stmt {
    StmtKind kind;
    int line;
    int col;

    union {
        struct {
            char *name;
            Expr *value;
        } assign;

        struct {
            Expr *expr;
        } expr;

        struct {
            Expr *cond;
            Stmt **then_body;
            size_t then_count;

            Stmt **else_body;
            size_t else_count;
        } if_stmt;

        struct {
            Expr *cond;
            Stmt **body;
            size_t body_count;
            int is_post; // 0 = do <cond> ... end
                         // 1 = do ... until <cond>
        } do_stmt;

        struct {
            char *name;
            char **params;
            size_t param_count;

            Stmt **body;
            size_t body_count;
        } fn_def;

        struct {
            Expr *value;
        } return_stmt;

    } as;
};

/* Program root */

typedef struct {
    Stmt **stmts;
    size_t count;
} Program;

void program_free(Program *program);

#endif
