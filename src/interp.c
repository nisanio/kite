#include "interp.h"
#include <stdio.h>
#include <stdlib.h>

// static int is_truthy(Value v) {
//     if (v.type == VAL_INT) return v.as.int_val != 0;
//     return 1;
// }

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

            break;
        }

        case EXPR_BINARY: {
            Value left = eval_expr(expr->as.binary.lhs, env);
            Value right = eval_expr(expr->as.binary.rhs, env);

            if (left.type != VAL_INT || right.type != VAL_INT) {
                printf("Binary operator requires integers\n");
                exit(1);
            }

            switch (expr->as.binary.op) {
                case BIN_ADD:
                    return value_int(left.as.int_val + right.as.int_val);
                case BIN_SUB:
                    return value_int(left.as.int_val - right.as.int_val);
                case BIN_MUL:
                    return value_int(left.as.int_val * right.as.int_val);
                case BIN_DIV:
                    return value_int(left.as.int_val / right.as.int_val);
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
