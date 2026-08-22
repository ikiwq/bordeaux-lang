#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm/emitter.h"
#include "file/file.h"
#include "mem/arena.h"

#include "analyzer/analyzer.h"
#include "scanner/scanner.h"
#include "parser/parser.h"

static bool parser_debug_mode = true;
static bool analyzer_debug_mode = true;

int main(const int argc, const char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: bdx <filepath>\n");
        exit(64);
    }

    arena_t *file_arena = arena_make(1 << 30u);
    const char *path = argv[1];
    char *src = read_file(file_arena, path);
    src_file_t src_file = {
        .path = path,
        .src = src,
        .len = strlen(src),
        .line_avec = line_avec_make(file_arena)
    };

    scanner_result_t scanner_res = scan(src_file);

    if(scanner_res.err_avec->size > 0) {
        print_fe_errs(src_file, scanner_res.err_avec);

        arena_destroy(scanner_res.arena);
        arena_destroy(file_arena);
        exit(64);
    }

    parser_result_t parser_res = parse(scanner_res.token_avec);
    arena_destroy(scanner_res.arena);

    if (parser_debug_mode) {
        printf("Parser output debug start\n\n");
        print_statements(parser_res.stmt_avec);
        printf("\nParser output debug end\n\n");
    }
    if (parser_res.err_avec->size> 0) {
        print_fe_errs(src_file, parser_res.err_avec);

        arena_destroy(parser_res.arena);
        arena_destroy(file_arena);
        exit(64);
    }

    analyzer_result_t analyzer_res = analyze(parser_res.stmt_avec);
    arena_destroy(parser_res.arena);

    if (analyzer_debug_mode) {
        printf("Analyzer output debug start\n\n");
        print_tstmts(analyzer_res.tstmt_avec);
        printf("\nAnalyzer output debug end\n\n");
    }
    if (analyzer_res.err_avec->size > 0) {
        print_fe_errs(src_file, analyzer_res.err_avec);

        arena_destroy(analyzer_res.arena);
        arena_destroy(file_arena);
        exit(64);
    }


    char target[256];
    strncpy(target, path, sizeof(target) - 1);
    target[sizeof(target) - 1] = '\0';
    char *dot = strrchr(target, '.');
    if (dot != NULL && (dot - target) < (256 - 5)) {
        strcpy(dot, ".asm");
    } else {
        strcat(target, ".asm"); 
    }

    emit_asm(target, analyzer_res.tstmt_avec);

    arena_destroy(analyzer_res.arena);
    arena_destroy(file_arena);

    return 0;
}
