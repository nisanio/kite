#include "interp.h"
#include <stdio.h>
#include <stdlib.h>



/* =========================
   Helpers
   ========================= */

static int is_bool(Value v) {
    return v.type == VAL_BOOL;
}

static int is_int(Value v) {
    return v.type == VAL_INT;
}

static void runtime_error(const char *msg) {
    printf("%s\n", msg);
    exit(1);
}



/* =========================
   Expr evaluation (split)
   ========================= */

static EvalResult eval_block(Stmt **stmts, size_t count, Env *env) {
    for (size_t i = 0; i < count; i++) {
        EvalResult r = eval_stmt(stmts[i], env);
        if (r.has_return) {
            return r;
        }
    }
    EvalResult ok = {0};
    return ok;
}

static Value eval_int_expr(Expr *expr) {
    return value_int(expr->as.int_val);
}

static Value eval_bool_expr(Expr *expr) {
    return value_bool(expr->as.bool_val);
}

static Value eval_var_expr(Expr *expr, Env *env) {
    Value v;
    if (!env_get(env, expr->as.var.name, &v)) {
        printf("Undefined variable: %s\n", expr->as.var.name);
        exit(1);
    }
    return v;
}

static Value eval_unary_expr(Expr *expr, Env *env) {
    Value right = eval_expr(expr->as.unary.rhs, env);

    if (expr->as.unary.op == UNOP_NEG) {
        if (!is_int(right)) {
            runtime_error("Unary '-' requires integer");
        }
        return value_int(-right.as.int_val);
    }

    if (expr->as.unary.op == UNOP_NOT) {
        if (!is_bool(right)) {
            runtime_error("'not' requires boolean");
        }
        return value_bool(!right.as.bool_val);
    }

    runtime_error("Unsupported unary operator");
    return value_int(0);
}

static Value eval_logical_binary(Expr *expr, Env *env) {
    /* Short-circuit logic */
    if (expr->as.binary.op == BIN_AND) {
        Value left = eval_expr(expr->as.binary.lhs, env);

        if (!is_bool(left)) {
            runtime_error("'and' requires boolean operands");
        }

        if (!left.as.bool_val) {
            return value_bool(0);
        }

        Value right = eval_expr(expr->as.binary.rhs, env);

        if (!is_bool(right)) {
            runtime_error("'and' requires boolean operands");
        }

        return value_bool(right.as.bool_val);
    }

    if (expr->as.binary.op == BIN_OR) {
        Value left = eval_expr(expr->as.binary.lhs, env);

        if (!is_bool(left)) {
            runtime_error("'or' requires boolean operands");
        }

        if (left.as.bool_val) {
            return value_bool(1);
        }

        Value right = eval_expr(expr->as.binary.rhs, env);

        if (!is_bool(right)) {
            runtime_error("'or' requires boolean operands");
        }

        return value_bool(right.as.bool_val);
    }

    runtime_error("Internal error: eval_logical_binary called for non-logical op");
    return value_bool(0);
}

static Value eval_arithmetic_binary(BinOp op, Value left, Value right) {
    if (!is_int(left) || !is_int(right)) {
        runtime_error("Arithmetic operators require integers");
    }

    switch (op) {
        case BIN_ADD: return value_int(left.as.int_val + right.as.int_val);
        case BIN_SUB: return value_int(left.as.int_val - right.as.int_val);
        case BIN_MUL: return value_int(left.as.int_val * right.as.int_val);
        case BIN_DIV: return value_int(left.as.int_val / right.as.int_val);
        default:
            runtime_error("Unsupported arithmetic operator");
            return value_int(0);
    }
}

static Value eval_comparison_binary(BinOp op, Value left, Value right) {
    if (!is_int(left) || !is_int(right)) {
        runtime_error("Comparison operators require integers");
    }

    int result = 0;

    switch (op) {
        case BIN_EQ:  result = (left.as.int_val == right.as.int_val); break;
        case BIN_NEQ: result = (left.as.int_val != right.as.int_val); break;
        case BIN_LT:  result = (left.as.int_val <  right.as.int_val); break;
        case BIN_LTE: result = (left.as.int_val <= right.as.int_val); break;
        case BIN_GT:  result = (left.as.int_val >  right.as.int_val); break;
        case BIN_GTE: result = (left.as.int_val >= right.as.int_val); break;
        default:
            runtime_error("Unsupported comparison operator");
            break;
    }

    return value_bool(result);
}

static Value eval_binary_expr(Expr *expr, Env *env) {
    BinOp op = expr->as.binary.op;

    /* Logical ops: short-circuit */
    if (op == BIN_AND || op == BIN_OR) {
        return eval_logical_binary(expr, env);
    }

    /* Non short-circuit: evaluate both sides */
    Value left = eval_expr(expr->as.binary.lhs, env);
    Value right = eval_expr(expr->as.binary.rhs, env);

    switch (op) {
        case BIN_ADD:
        case BIN_SUB:
        case BIN_MUL:
        case BIN_DIV:
            return eval_arithmetic_binary(op, left, right);

        case BIN_EQ:
        case BIN_NEQ:
        case BIN_LT:
        case BIN_LTE:
        case BIN_GT:
        case BIN_GTE:
            return eval_comparison_binary(op, left, right);

        default:
            runtime_error("Unsupported binary operator");
            return value_int(0);
    }
}

static EvalResult eval_do_stmt(Stmt *stmt, Env *env) {

    if (!stmt->as.do_stmt.is_post) {
        /* while-style */
        while (1) {
            Value cond = eval_expr(stmt->as.do_stmt.cond, env);

            if (!is_bool(cond))
                runtime_error("do condition must be boolean");

            if (!cond.as.bool_val)
                break;

            EvalResult r = eval_block(stmt->as.do_stmt.body,
                                      stmt->as.do_stmt.body_count,
                                      env);

            if (r.has_return)
                return r;
        }

        EvalResult ok = {0};
        return ok;
    }

    /* repeat-style */
    while (1) {
        EvalResult r = eval_block(stmt->as.do_stmt.body,
                                  stmt->as.do_stmt.body_count,
                                  env);

        if (r.has_return)
            return r;

        Value cond = eval_expr(stmt->as.do_stmt.cond, env);

        if (!is_bool(cond))
            runtime_error("until condition must be boolean");

        if (cond.as.bool_val)
            break;
    }

    EvalResult ok = {0};
    return ok;
}


static EvalResult eval_return_stmt(Stmt *stmt, Env *env) {
    if (stmt->as.return_stmt.value == NULL) {
        runtime_error("return requires a value");
    }

    Value v = eval_expr(stmt->as.return_stmt.value, env);

    EvalResult r;
    r.has_return = 1;
    r.value = v;
    return r;
}

static Value eval_call_expr(Expr *expr, Env *env) {

    Value callee;

    if (!env_get(env, expr->as.call.callee, &callee)) {
        printf("Undefined function: %s\n", expr->as.call.callee);
        exit(1);
    }

    if (callee.type != VAL_FUNCTION) {
        runtime_error("Attempt to call non-function value");
    }

    Function *fn = callee.as.fn_val;

    if (expr->as.call.argc != fn->param_count) {
        runtime_error("Argument count mismatch");
    }

    /* Create new environment for invocation */
    Env *local = env_create(fn->closure);

    /* Bind parameters */
    for (size_t i = 0; i < fn->param_count; i++) {
        Value arg = eval_expr(expr->as.call.args[i], env);
        env_define(local, fn->params[i], arg);
    }

    /* Execute function body */
    EvalResult result = eval_block(fn->body, fn->body_count, local);

    env_free(local);

    if (result.has_return) {
        return result.value;
    }

    /* No explicit return */
    runtime_error("Function returned without value");
    return value_int(0);  /* unreachable */
}




Value eval_expr(Expr *expr, Env *env) {
    switch (expr->kind) {
        case EXPR_INT:
            return eval_int_expr(expr);

        case EXPR_BOOL:
            return eval_bool_expr(expr);

        case EXPR_VAR:
            return eval_var_expr(expr, env);

        case EXPR_UNARY:
            return eval_unary_expr(expr, env);

        case EXPR_BINARY:
            return eval_binary_expr(expr, env);
        
        case EXPR_CALL:
            return eval_call_expr(expr, env);

        default:
            runtime_error("Unsupported expression");
            return value_int(0);
    }
}

/* =========================
   Statement evaluation (split)
   ========================= */

static void eval_assign_stmt(Stmt *stmt, Env *env) {
    Value value = eval_expr(stmt->as.assign.value, env);

    if (!env_assign(env, stmt->as.assign.name, value)) {
        env_define(env, stmt->as.assign.name, value);
    }
}

static void eval_expr_stmt(Stmt *stmt, Env *env) {
    Value value = eval_expr(stmt->as.expr.expr, env);

    if (value.type == VAL_INT) {
        printf("=> %lld\n", (long long)value.as.int_val);
        return;
    }

    if (value.type == VAL_BOOL) {
        printf("=> %s\n", value.as.bool_val ? "true" : "false");
        return;
    }

    printf("=> <non-printable>\n");
}

static EvalResult eval_if_stmt(Stmt *stmt, Env *env) {
    Value cond = eval_expr(stmt->as.if_stmt.cond, env);

    if (!is_bool(cond)) {
        runtime_error("if condition must be boolean");
    }

    if (cond.as.bool_val) {
        return eval_block(stmt->as.if_stmt.then_body,
                          stmt->as.if_stmt.then_count,
                          env);
    } else {
        return eval_block(stmt->as.if_stmt.else_body,
                          stmt->as.if_stmt.else_count,
                          env);
    }
}


EvalResult eval_stmt(Stmt *stmt, Env *env) {

    switch (stmt->kind) {

        case STMT_ASSIGN:
            eval_assign_stmt(stmt, env);
            break;

        case STMT_EXPR:
            eval_expr_stmt(stmt, env);
            break;

        case STMT_IF:
            return eval_if_stmt(stmt, env);

        case STMT_DO:
            return eval_do_stmt(stmt, env);

        case STMT_RETURN:
            return eval_return_stmt(stmt, env);

        case STMT_FNDEF:
            eval_fn_def_stmt(stmt, env);
        break;
        
        default:
            runtime_error("Unsupported statement");
            break;
    }

    EvalResult ok = {0};
    return ok;
}

static void eval_fn_def_stmt(Stmt *stmt, Env *env) {

    Function *fn = malloc(sizeof(Function));
    if (!fn) {
        runtime_error("Out of memory allocating function");
    }

    fn->params = stmt->as.fn_def.params;
    fn->param_count = stmt->as.fn_def.param_count;
    fn->body = stmt->as.fn_def.body;
    fn->body_count = stmt->as.fn_def.body_count;
    fn->closure = env;  // capture lexical environment

    Value v;
    v.type = VAL_FUNCTION;
    v.as.fn_val = fn;

    env_define(env, stmt->as.fn_def.name, v);
}



EvalResult eval_program(Program *program, Env *env) {
    for (size_t i = 0; i < program->count; i++) {
        EvalResult r = eval_stmt(program->stmts[i], env);
        if (r.has_return) {
            runtime_error("return is only valid inside functions");
        }
    }

    EvalResult ok = {0};
    return ok;
}
