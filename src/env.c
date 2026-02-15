#include "env.h"
#include <stdlib.h>
#include <string.h>



typedef struct EnvEntry {
    char *name;
    Value value;
    struct EnvEntry *next;
} EnvEntry;

struct Env {
    EnvEntry *head;
    Env *parent;
};

/* Create */

Env *env_create(Env *parent) {
    Env *env = malloc(sizeof(Env));
    if (!env) exit(1);
    env->head = NULL;
    env->parent = parent;
    return env;
}

/* Free */

void env_free(Env *env) {
    EnvEntry *entry = env->head;
    while (entry) {
        EnvEntry *next = entry->next;
        free(entry->name);
        value_free(entry->value);
        free(entry);
        entry = next;
    }
    free(env);
}

/* Define (always in current scope) */

void env_define(Env *env, const char *name, Value value) {
    EnvEntry *entry = malloc(sizeof(EnvEntry));
    if (!entry) exit(1);

    entry->name = strdup(name);
    entry->value = value_clone(value); /* Env owns stored values */
    entry->next = env->head;
    env->head = entry;
}

/* Assign (search up chain) */

int env_assign(Env *env, const char *name, Value value) {
    for (Env *e = env; e != NULL; e = e->parent) {
        EnvEntry *entry = e->head;
        while (entry) {
            if (strcmp(entry->name, name) == 0) {
                value_free(entry->value);
                entry->value = value_clone(value); /* Env owns stored values */
                return 1;
            }
            entry = entry->next;
        }
    }
    return 0;
}

/* Get (search up chain) */

int env_get(Env *env, const char *name, Value *out) {
    for (Env *e = env; e != NULL; e = e->parent) {
        EnvEntry *entry = e->head;
        while (entry) {
            if (strcmp(entry->name, name) == 0) {
                *out = entry->value; /* return by value (non-owning copy) */
                return 1;
            }
            entry = entry->next;
        }
    }
    return 0;
}

int env_has_local(Env *env, const char *name) {
    EnvEntry *entry = env->head;
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}


extern Value builtin_print(Value *args, size_t argc);
extern Value builtin_len(Value *args, size_t argc);
extern Value builtin_read_file(Value *args, size_t argc);
extern Value builtin_write_file(Value *args, size_t argc);
Env *env_create_global(void) {
    Env *env = env_create(NULL);

    Value v;
    v.type = VAL_BUILTIN;
    v.as.builtin_val = builtin_print;

    env_define(env, "print", v);

    Value v2;
    v2.type = VAL_BUILTIN;
    v2.as.builtin_val = builtin_len;

    env_define(env, "len", v2);

    Value v3;
    v3.type = VAL_BUILTIN;
    v3.as.builtin_val = builtin_read_file;

    env_define(env, "read_file", v3);

    Value v4;
    v4.type = VAL_BUILTIN;
    v4.as.builtin_val = builtin_write_file;

    env_define(env, "write_file", v4);

    return env;
}
