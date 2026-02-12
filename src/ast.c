#include "ast.h"
#include <stdlib.h>

/* =========================
   Forward declarations
   ========================= */

static void expr_free(Expr *expr);
static void stmt_free(Stmt *stmt);

/* =========================
   Expression free
   ========================= */

static void expr_free(Expr *expr) {
    if (!expr) return;

    switch (expr->kind) {

        case EXPR_STRING:
            free(expr->as.string.data);
            break;

        case EXPR_VAR:
            free(expr->as.var.name);
            break;

        case EXPR_ARRAY:
            for (size_t i = 0; i < expr->as.array.count; i++) {
                expr_free(expr->as.array.items[i]);
            }
            free(expr->as.array.items);
            break;

        case EXPR_CALL:
            free(expr->as.call.callee);
            for (size_t i = 0; i < expr->as.call.argc; i++) {
                expr_free(expr->as.call.args[i]);
            }
            free(expr->as.call.args);
            break;

        case EXPR_INDEX:
            expr_free(expr->as.index.base);
            expr_free(expr->as.index.index);
            break;

        case EXPR_UNARY:
            expr_free(expr->as.unary.rhs);
            break;

        case EXPR_BINARY:
            expr_free(expr->as.binary.lhs);
            expr_free(expr->as.binary.rhs);
            break;

        case EXPR_INT:
        default:
            break;
    }

    free(expr);
}

/* =========================
   Statement free
   ========================= */

static void stmt_free(Stmt *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {

        case STMT_ASSIGN:
            free(stmt->as.assign.name);
            expr_free(stmt->as.assign.value);
            break;

        case STMT_EXPR:
            expr_free(stmt->as.expr.expr);
            break;

        case STMT_IF:
            expr_free(stmt->as.if_stmt.cond);

            for (size_t i = 0; i < stmt->as.if_stmt.then_count; i++) {
                stmt_free(stmt->as.if_stmt.then_body[i]);
            }
            free(stmt->as.if_stmt.then_body);

            for (size_t i = 0; i < stmt->as.if_stmt.else_count; i++) {
                stmt_free(stmt->as.if_stmt.else_body[i]);
            }
            free(stmt->as.if_stmt.else_body);
            break;

        case STMT_DO:
            expr_free(stmt->as.do_stmt.cond);
            for (size_t i = 0; i < stmt->as.do_stmt.body_count; i++) {
                stmt_free(stmt->as.do_stmt.body[i]);
            }
            free(stmt->as.do_stmt.body);
            break;

        case STMT_FNDEF:
            free(stmt->as.fn_def.name);

            for (size_t i = 0; i < stmt->as.fn_def.param_count; i++) {
                free(stmt->as.fn_def.params[i]);
            }
            free(stmt->as.fn_def.params);

            for (size_t i = 0; i < stmt->as.fn_def.body_count; i++) {
                stmt_free(stmt->as.fn_def.body[i]);
            }
            free(stmt->as.fn_def.body);
            break;

        case STMT_RETURN:
            expr_free(stmt->as.return_stmt.value);
            break;

        default:
            break;
    }

    free(stmt);
}

/* =========================
   Program free
   ========================= */

void program_free(Program *program) {
    if (!program) return;

    for (size_t i = 0; i < program->count; i++) {
        stmt_free(program->stmts[i]);
    }

    free(program->stmts);
    free(program);
}
