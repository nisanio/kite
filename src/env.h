#ifndef ENV_H
#define ENV_H

#include "value.h"

typedef struct Env Env;

Env *env_create(Env *parent);
void env_free(Env *env);

void env_define(Env *env, const char *name, Value value);
int  env_assign(Env *env, const char *name, Value value);
int  env_get(Env *env, const char *name, Value *out);
int env_has_local(Env *env, const char *name);
Env *env_create_global(void);
#endif
