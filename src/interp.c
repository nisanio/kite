#include "interp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Forward (needed because eval_stmt uses it before definition) */
static void eval_fn_def_stmt(Stmt *stmt, Env *env);
static void runtime_error(const char *msg);
/* =========================
   Helpers
   ========================= */

 Value builtin_print(Value *args, size_t argc) {
    if (argc != 1) {
        runtime_error("print expects exactly one argument");
    }

    Value v = args[0];

    if (v.type == VAL_STRING) {
        printf("%s\n", v.as.str_val.data);
        return value_bool(true);
    }

    if (v.type == VAL_INT) {
        printf("%lld\n", (long long)v.as.int_val);
        return value_bool(true);
    }

    if (v.type == VAL_BOOL) {
        printf("%s\n", v.as.bool_val ? "true" : "false");
        return value_bool(true);
    }

    runtime_error("Unsupported type for print");
    return value_bool(false);
}

Value builtin_len(Value *args, size_t argc) {
    if (argc != 1) {
        runtime_error("len expects exactly one argument");
    }

    Value v = args[0];

    if (v.type != VAL_STRING) {
        runtime_error("len expects a string");
    }

    return value_int((int64_t)v.as.str_val.len);
}


static int is_bool(Value v) {
    return v.type == VAL_BOOL;
}

static int is_int(Value v) {
    return v.type == VAL_INT;
}


static void runtime_error_stmt(Stmt *stmt, const char *msg) {
    printf("[line %d:%d] %s\n", stmt->line, stmt->col, msg);
    exit(1);
}
    

static void runtime_error(const char *msg) {
    printf("%s\n", msg);
    exit(1);
}

static void runtime_error_at(int line, int col, const char *msg) {
    printf("[line %d, col %d] %s\n", line, col, msg);
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
        runtime_error_at(expr->line, expr->col,
                         "Undefined variable");
    }
    return v;
}

static Value eval_index_expr(Expr *expr, Env *env) {
    Value base = eval_expr(expr->as.index.base, env);

    if (base.type != VAL_ARRAY) {
        runtime_error("Indexing requires array");
    }

    Value index = eval_expr(expr->as.index.index, env);

    if (index.type != VAL_INT) {
        runtime_error("Array index must be integer");
    }

    int64_t i = index.as.int_val;

    if (i < 0 || (size_t)i >= base.as.array_val.count) {
        runtime_error("Array index out of bounds");
    }

    return base.as.array_val.items[i];
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

static Value eval_array_expr(Expr *expr, Env *env) {
    Value arr = value_array();

    size_t count = expr->as.array.count;

    arr.as.array_val.items = malloc(sizeof(Value) * count);
    if (!arr.as.array_val.items) {
        runtime_error_at(expr->line, expr->col, "Out of memory");
    }

    arr.as.array_val.count = count;
    arr.as.array_val.capacity = count;

    for (size_t i = 0; i < count; i++) {
        arr.as.array_val.items[i] =
            eval_expr(expr->as.array.items[i], env);
    }

    return arr;
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

static Value eval_arithmetic_binary(Expr *expr, Value left, Value right){
    if (!is_int(left) || !is_int(right)) {
        runtime_error("Arithmetic operators require integers");
    }

    switch (expr->as.binary.op) {
        case BIN_ADD:
            return value_int(left.as.int_val + right.as.int_val);
        case BIN_SUB:
            return value_int(left.as.int_val - right.as.int_val);
        case BIN_MUL:
            return value_int(left.as.int_val * right.as.int_val);
        case BIN_DIV:
            if (right.as.int_val == 0) {
               runtime_error_at(expr->line, expr->col, "Division by zero");
            }
            return value_int(left.as.int_val / right.as.int_val);
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
        case BIN_EQ:
            result = (left.as.int_val == right.as.int_val);
            break;
        case BIN_NEQ:
            result = (left.as.int_val != right.as.int_val);
            break;
        case BIN_LT:
            result = (left.as.int_val < right.as.int_val);
            break;
        case BIN_LTE:
            result = (left.as.int_val <= right.as.int_val);
            break;
        case BIN_GT:
            result = (left.as.int_val > right.as.int_val);
            break;
        case BIN_GTE:
            result = (left.as.int_val >= right.as.int_val);
            break;
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
            if (left.type == VAL_STRING && right.type == VAL_STRING) {
                size_t len = left.as.str_val.len + right.as.str_val.len;

                char *buffer = malloc(len + 1);
                if (!buffer) {
                    runtime_error_at(expr->line, expr->col, "Out of memory");
                }

                memcpy(buffer, left.as.str_val.data, left.as.str_val.len);
                memcpy(buffer + left.as.str_val.len,
                    right.as.str_val.data,
                    right.as.str_val.len);

                buffer[len] = '\0';

                Value v;
                v.type = VAL_STRING;
                v.as.str_val.data = buffer;
                v.as.str_val.len = len;
                return v;
            }
            return eval_arithmetic_binary(expr, left, right);
        case BIN_SUB:
        case BIN_MUL:
        case BIN_DIV:
            return eval_arithmetic_binary(expr, left, right);

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
        Value args[expr->as.call.argc];

        for (size_t i = 0; i < expr->as.call.argc; i++) {
            args[i] = eval_expr(expr->as.call.args[i], env);
        }

        return callee.as.builtin_val(args, expr->as.call.argc);
    }


    Function *fn = callee.as.fn_val;

   if (expr->as.call.argc != fn->param_count) {
        runtime_error_at(expr->line, expr->col,
                        "Argument count mismatch");
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
    return value_int(0); /* unreachable */
}

Value eval_expr(Expr *expr, Env *env) {
    switch (expr->kind) {
        case EXPR_INT:
            return eval_int_expr(expr);

        case EXPR_STRING:
            return value_string(expr->as.string.data);

        case EXPR_BOOL:
            return eval_bool_expr(expr);

        case EXPR_VAR:
            return eval_var_expr(expr, env);

        case EXPR_UNARY:
            return eval_unary_expr(expr, env);

        case EXPR_BINARY:
            return eval_binary_expr(expr, env);

        case EXPR_ARRAY:
            return eval_array_expr(expr, env);

        case EXPR_CALL:
            return eval_call_expr(expr, env);
        
        case EXPR_INDEX:
            return eval_index_expr(expr, env);

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

    Expr *expr = stmt->as.expr.expr;

    Value value = eval_expr(expr, env);

    /* Do not print result of print() calls */
    if (expr->kind == EXPR_CALL &&
        strcmp(expr->as.call.callee, "print") == 0) {
        return;
    }

    if (value.type == VAL_INT) {
        printf("=> %lld\n", (long long)value.as.int_val);
        return;
    }

    if (value.type == VAL_BOOL) {
        printf("=> %s\n", value.as.bool_val ? "true" : "false");
        return;
    }

    if (value.type == VAL_STRING) {
        printf("=> %s\n", value.as.str_val.data);
        return;
    }

    if (value.type == VAL_ARRAY) {
        printf("=> [");

        for (size_t i = 0; i < value.as.array_val.count; i++) {
            Value item = value.as.array_val.items[i];

            if (item.type == VAL_INT) {
                printf("%lld", (long long)item.as.int_val);
            } else if (item.type == VAL_STRING) {
                printf("\"%s\"", item.as.str_val.data);
            } else if (item.type == VAL_BOOL) {
                printf("%s", item.as.bool_val ? "true" : "false");
            } else {
                printf("<unsupported>");
            }

            if (i + 1 < value.as.array_val.count)
                printf(", ");
        }

        printf("]\n");
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
    /* Create a temporary wrapper; env_define will clone it (heap-owning). */
    if (env_has_local(env, stmt->as.fn_def.name)) {
        runtime_error_at(stmt->line, stmt->col,
                         "Function redefinition not allowed");
    }
    Function tmp;
    tmp.params = stmt->as.fn_def.params;
    tmp.param_count = stmt->as.fn_def.param_count;
    tmp.body = stmt->as.fn_def.body;
    tmp.body_count = stmt->as.fn_def.body_count;
    tmp.closure = env;

    Value v;
    v.type = VAL_FUNCTION;
    v.as.fn_val = &tmp;

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
