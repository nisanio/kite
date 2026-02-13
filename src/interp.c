#include "interp.h"
#include <stdio.h>
#include <stdlib.h>

static int is_bool(Value v) {
    return v.type == VAL_BOOL;
}

Value eval_expr(Expr *expr, Env *env) {
    switch (expr->kind) {

        case EXPR_INT:
            return value_int(expr->as.int_val);

        case EXPR_VAR: {
            Value v;
            if (!env_get(env, expr->as.var.name, &v)) {
                printf("Undefined variable: %s\n", expr->as.var.name);
                exit(1);
            }
            return v;
        }

        case EXPR_UNARY: {
            Value right = eval_expr(expr->as.unary.rhs, env);

            if (expr->as.unary.op == UNOP_NEG) {
                if (right.type != VAL_INT) {
                    printf("Unary '-' requires integer\n");
                    exit(1);
                }
                return value_int(-right.as.int_val);
            }

            if (expr->as.unary.op == UNOP_NOT) {
                if (!is_bool(right)) {
                    printf("'not' requires boolean\n");
                    exit(1);
                }
                return value_bool(!right.as.bool_val);
            }

            break;
        }

        case EXPR_BINARY: {

            /* Short-circuit logic */
            if (expr->as.binary.op == BIN_AND) {
                Value left = eval_expr(expr->as.binary.lhs, env);

                if (!is_bool(left)) {
                    printf("'and' requires boolean operands\n");
                    exit(1);
                }

                if (!left.as.bool_val) {
                    return value_bool(0);
                }

                Value right = eval_expr(expr->as.binary.rhs, env);

                if (!is_bool(right)) {
                    printf("'and' requires boolean operands\n");
                    exit(1);
                }

                return value_bool(right.as.bool_val);
            }

            if (expr->as.binary.op == BIN_OR) {
                Value left = eval_expr(expr->as.binary.lhs, env);

                if (!is_bool(left)) {
                    printf("'or' requires boolean operands\n");
                    exit(1);
                }

                if (left.as.bool_val) {
                    return value_bool(1);
                }

                Value right = eval_expr(expr->as.binary.rhs, env);

                if (!is_bool(right)) {
                    printf("'or' requires boolean operands\n");
                    exit(1);
                }

                return value_bool(right.as.bool_val);
            }

            /* Non short-circuit operators */
            Value left = eval_expr(expr->as.binary.lhs, env);
            Value right = eval_expr(expr->as.binary.rhs, env);

            switch (expr->as.binary.op) {

                case BIN_ADD:
                case BIN_SUB:
                case BIN_MUL:
                case BIN_DIV: {
                    if (left.type != VAL_INT || right.type != VAL_INT) {
                        printf("Arithmetic operators require integers\n");
                        exit(1);
                    }

                    switch (expr->as.binary.op) {
                        case BIN_ADD: return value_int(left.as.int_val + right.as.int_val);
                        case BIN_SUB: return value_int(left.as.int_val - right.as.int_val);
                        case BIN_MUL: return value_int(left.as.int_val * right.as.int_val);
                        case BIN_DIV: return value_int(left.as.int_val / right.as.int_val);
                        default: break;
                    }
                }

                case BIN_EQ:
                case BIN_NEQ:
                case BIN_LT:
                case BIN_LTE:
                case BIN_GT:
                case BIN_GTE: {
                    if (left.type != VAL_INT || right.type != VAL_INT) {
                        printf("Comparison operators require integers\n");
                        exit(1);
                    }

                    int result = 0;

                    switch (expr->as.binary.op) {
                        case BIN_EQ:  result = (left.as.int_val == right.as.int_val); break;
                        case BIN_NEQ: result = (left.as.int_val != right.as.int_val); break;
                        case BIN_LT:  result = (left.as.int_val <  right.as.int_val); break;
                        case BIN_LTE: result = (left.as.int_val <= right.as.int_val); break;
                        case BIN_GT:  result = (left.as.int_val >  right.as.int_val); break;
                        case BIN_GTE: result = (left.as.int_val >= right.as.int_val); break;
                        default: break;
                    }

                    return value_bool(result);
                }

                default:
                    break;
            }

            break;
        }

        default:
            break;
    }

    printf("Unsupported expression\n");
    exit(1);
}

void eval_stmt(Stmt *stmt, Env *env) {
    switch (stmt->kind) {

        case STMT_ASSIGN: {
            Value value = eval_expr(stmt->as.assign.value, env);

            if (!env_assign(env, stmt->as.assign.name, value)) {
                env_define(env, stmt->as.assign.name, value);
            }
            break;
        }

        case STMT_EXPR: {
            Value value = eval_expr(stmt->as.expr.expr, env);

            if (value.type == VAL_INT) {
                printf("=> %lld\n", (long long)value.as.int_val);
            } else if (value.type == VAL_BOOL) {
                printf("=> %s\n", value.as.bool_val ? "true" : "false");
            } else {
                printf("=> <non-printable>\n");
            }

            break;
        }

        default:
            printf("Unsupported statement\n");
            exit(1);
    }
}

void eval_program(Program *program, Env *env) {
    for (size_t i = 0; i < program->count; i++) {
        eval_stmt(program->stmts[i], env);
    }
}
