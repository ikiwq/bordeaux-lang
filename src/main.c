#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "checker/checker.h"
#include "common.h"
#include "file.h"
#include "mem/arena.h"
#include "mem/pvec.h"
#include "parser/parser.h"
#include "parser/stmt.h"
#include "transpiler/transpiler.h"

int main(const int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: bdx <filepath>\n");
        exit(64);
    }

    arena_t *arena = arena_make(1 << 28u);

    const char *stdlib_path = "stdlib/mod.bdx";
    char *stdlib_src = read_file(stdlib_path);
    errors_init((src_file_t){
        .path = stdlib_path,
        .src = stdlib_src,
        .len = strlen(stdlib_src),
    });
    parse_result_t stdlib_parser_res = parse(stdlib_src, arena);

    const char *path = argv[1];
    char *src = read_file(path);
    errors_init((src_file_t){
        .path = path,
        .src = src,
        .len = strlen(src),
    });
    parse_result_t parser_res = parse(src, arena);
    if (!parser_res.success) {
        fprintf(stderr, "error parsing file");
        exit(1);
    }

    pvec_t *vec = pvec_init();
    for (size_t i = 0; i < stdlib_parser_res.stmt_count; i++) {
        pvec_push(vec, stdlib_parser_res.stmts[i]);
    }
    for (size_t i = 0; i < parser_res.stmt_count; i++) {
        pvec_push(vec, parser_res.stmts[i]);
    }

    bool checker_res = check((stmt_t **)vec->items, vec->size);
    if (!checker_res) {
        fprintf(stderr, "error checking file");
        exit(1);
    }

    transpile((stmt_t **)vec->items, vec->size);

    system("gcc -Wall main.c runtime/runtime.c -o out");

    arena_destroy(arena);
    free(src);

    return 0;
}
