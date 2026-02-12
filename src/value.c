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

/* ===== Internal helpers ===== */

static void array_free(Array *arr) {
    for (size_t i = 0; i < arr->count; i++) {
        value_free(arr->items[i]);
    }
    free(arr->items);
}

/* ===== Memory management ===== */

void value_free(Value v) {
    switch (v.type) {
        case VAL_STRING:
            free(v.as.str_val.data);
            break;

        case VAL_ARRAY:
            array_free(&v.as.array_val);
            break;

        case VAL_FUNCTION:
            /* Managed elsewhere */
            break;

        case VAL_BUILTIN:
            /* Nothing to free */
            break;

        case VAL_INT:
        default:
            break;
    }
}
