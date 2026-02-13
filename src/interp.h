#ifndef INTERP_H
#define INTERP_H

#include "ast.h"
#include "env.h"
#include "value.h"

Value eval_expr(Expr *expr, Env *env);
void eval_stmt(Stmt *stmt, Env *env);
void eval_program(Program *program, Env *env);

#endif
