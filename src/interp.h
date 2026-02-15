#ifndef INTERP_H
#define INTERP_H

#include "ast.h"
#include "env.h"
#include "value.h"

typedef struct {
    int has_return;
    Value value;
} EvalResult;


Value eval_expr(Expr *expr, Env *env);
EvalResult eval_stmt(Stmt *stmt, Env *env);
EvalResult eval_program(Program *program, Env *env);

#endif
