#include "lexer.h"
#include "parser.h"
#include "interp.h"
#include "env.h"
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

static char *read_all(FILE *fp) {
    size_t cap = 4096;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    for (;;) {
        size_t avail = cap - len;
        if (avail < 1024) {
            cap *= 2;
            char *nb = (char *)realloc(buf, cap);
            if (!nb) {
                free(buf);
                fprintf(stderr, "out of memory\n");
                exit(1);
            }
            buf = nb;
            avail = cap - len;
        }

        size_t n = fread(buf + len, 1, avail, fp);
        len += n;

        if (n == 0) {
            if (feof(fp)) break;
            fprintf(stderr, "read error\n");
            free(buf);
            exit(1);
        }
    }

    buf = (char *)realloc(buf, len + 1);
    buf[len] = '\0';
    return buf;
}

int main(int argc, char **argv) {
    FILE *fp = stdin;

    if (argc == 2) {
        fp = fopen(argv[1], "rb");
        if (!fp) {
            fprintf(stderr, "cannot open file: %s\n", argv[1]);
            return 1;
        }
    } else if (argc > 2) {
        fprintf(stderr, "usage: %s [file]\n", argv[0]);
        return 1;
    }

    char *source = read_all(fp);

    if (fp != stdin) {
        fclose(fp);
    }

    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    Program *program = parse_program(&parser);

    Env *global = env_create(NULL);

    (void)eval_program(program, global);

    program_free(program);
    env_free(global);
    free(source);

    return 0;
}
