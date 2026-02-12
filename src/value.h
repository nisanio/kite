#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    VAL_INT,
    VAL_STRING,
    VAL_ARRAY,
    VAL_FUNCTION,
    VAL_BUILTIN
} ValueType;

typedef struct Value Value;
typedef struct Function Function;

typedef Value (*BuiltinFn)(Value *args, size_t argc);

typedef struct {
    char *data;
    size_t len;
} String;

typedef struct {
    Value *items;
    size_t count;
    size_t capacity;
} Array;

struct Value {
    ValueType type;
    union {
        int64_t int_val;
        String str_val;
        Array array_val;
        Function *fn_val;
        BuiltinFn builtin_val;
    } as;
};

/* Constructors */
Value value_int(int64_t x);
Value value_string(const char *s);
Value value_array(void);

/* Memory management */
void value_free(Value v);

#endif
