#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem/arena.h"
#include "checker/checker.h"
#include "common.h"
#include "file.h"
#include "parser/debug.h"
#include "parser/parser.h"
#include "ssa/gen.h"

int main(const int argc, const char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: bdx <filepath>\n");
        exit(64);
    }

    const char *path = argv[1];
    char *src = read_file(path);

    errors_init((src_file_t) {
        .path = path,
        .src = src,
        .len = strlen(src),
    });

    arena_t *arena = arena_make(1 << 28u);

    parse_result_t parser_res = parse(src, arena);
    if(!parser_res.success) {
        fprintf(stderr, "error parsing file");
        exit(1);
    }
    printf("\n=== AST ===\n\n");
    print_stmts(parser_res.stmts, parser_res.stmt_count);

    bool checker_res = check(parser_res.stmts, parser_res.stmt_count);
    if(!checker_res) {
        fprintf(stderr, "error checking file");
        exit(1);
    }
    printf("\n=== Typed AST ===\n\n");
    print_stmts(parser_res.stmts, parser_res.stmt_count);

    gen_result_t gen_res = gen_ssa(parser_res.stmts, parser_res.stmt_count);

    free(src);

    return 0;
}
