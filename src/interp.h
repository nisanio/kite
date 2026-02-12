#ifndef INTERP_H
#define INTERP_H

#include "ast.h"
#include "env.h"
#include "value.h"

Value eval_expr(Expr *expr, Env *env);

#endif
