#include "value.h"
#include <stdlib.h>
#include <string.h>

/* ===== Constructors ===== */

Value value_int(int64_t x) {
    Value v;
    v.type = VAL_INT;
    v.as.int_val = x;
    return v;
}

Value value_string(const char *s) {
    Value v;
    v.type = VAL_STRING;

    size_t len = strlen(s);
    v.as.str_val.data = malloc(len + 1);
    if (!v.as.str_val.data) {
        exit(1);
    }

    memcpy(v.as.str_val.data, s, len + 1);
    v.as.str_val.len = len;

    return v;
}

Value value_array(void) {
    Value v;
    v.type = VAL_ARRAY;
    v.as.array_val.items = NULL;
    v.as.array_val.count = 0;
    v.as.array_val.capacity = 0;
    return v;
}

Value value_bool(bool b) {
    Value v;
    v.type = VAL_BOOL;
    v.as.bool_val = b;
    return v;
}

/* ===== Internal helpers ===== */

static void array_free(Array *arr) {
    for (size_t i = 0; i < arr->count; i++) {
        value_free(arr->items[i]);
    }
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

static Array array_clone(const Array *src) {
    Array dst;
    dst.count = src->count;
    dst.capacity = src->count;
    dst.items = NULL;

    if (src->count == 0) {
        return dst;
    }

    dst.items = (Value *)malloc(sizeof(Value) * src->count);
    if (!dst.items) {
        exit(1);
    }

    for (size_t i = 0; i < src->count; i++) {
        dst.items[i] = value_clone(src->items[i]);
    }

    return dst;
}

/* ===== Memory management ===== */

Value value_clone(Value v) {
    Value out;
    out.type = v.type;

    switch (v.type) {
        case VAL_INT:
            out.as.int_val = v.as.int_val;
            break;

        case VAL_BOOL:
            out.as.bool_val = v.as.bool_val;
            break;

        case VAL_BUILTIN:
            out.as.builtin_val = v.as.builtin_val;
            break;

        case VAL_STRING: {
            size_t len = v.as.str_val.len;
            out.as.str_val.data = (char *)malloc(len + 1);
            if (!out.as.str_val.data) {
                exit(1);
            }
            memcpy(out.as.str_val.data, v.as.str_val.data, len + 1);
            out.as.str_val.len = len;
        } break;

        case VAL_ARRAY:
            out.as.array_val = array_clone(&v.as.array_val);
            break;

        case VAL_FUNCTION: {
            Function *src = v.as.fn_val;
            Function *fn = (Function *)malloc(sizeof(Function));
            if (!fn) {
                exit(1);
            }

            /* Shallow copy of function "shape": params/body belong to AST.
               Wrapper is owned by Value/Env and must be freed. */
            fn->params = src->params;
            fn->param_count = src->param_count;
            fn->body = src->body;
            fn->body_count = src->body_count;
            fn->closure = src->closure;

            out.as.fn_val = fn;
        } break;

        default:
            /* Should not happen */
            break;
    }

    return out;
}

void value_free(Value v) {
    switch (v.type) {
        case VAL_STRING:
            free(v.as.str_val.data);
            break;

        case VAL_ARRAY:
            array_free(&v.as.array_val);
            break;

        case VAL_FUNCTION:
            free(v.as.fn_val);
            break;

        case VAL_BUILTIN:
            /* Nothing to free */
            break;

        case VAL_BOOL:
            /* Nothing to free */
            break;

        case VAL_INT:
        default:
            break;
    }
}
