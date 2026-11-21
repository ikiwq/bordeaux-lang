#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "checker/checker.h"
#include "common.h"
#include "file.h"
#include "mem/arena.h"
#include "parser/parser.h"
#include "transpiler/transpiler.h"

int main(const int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: bdx <filepath>\n");
        exit(64);
    }

    arena_t *arena = arena_make(1 << 28u);

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

    bool checker_res = check(parser_res.statements);
    if (!checker_res) {
        fprintf(stderr, "error checking file");
        exit(1);
    }

    transpile(parser_res.statements);

    system("gcc -Wall main.c runtime/runtime.c -o out");

    arena_destroy(arena);
    free(src);

    return 0;
}
