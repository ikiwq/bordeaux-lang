#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analyzer/analyzer.h"
#include "file/file.h"
#include "scanner/scanner.h"
#include "parser/parser.h"

static bool parser_debug_mode = true;
static bool analyzer_debug_mode = true;

int main(const int argc, const char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: bdx <filepath>\n");
        exit(64);
    }

    const char *path = argv[1];
    char *src = read_file(path);
    src_file_t src_file = (src_file_t) {
        .path = path,
        .src = src,
        .len = strlen(src),
        .line_vec = line_vec_init()
    };

    scanner_result_t scanner_res = scan(&src_file);

    if (scanner_res.err_count > 0) {
        for (size_t i = 0; i < scanner_res.err_count; i++) {
            print_fe_err(src_file, scanner_res.errs[i]);
        }
        arena_destroy(&scanner_res.arena);
        exit(64);
    }

    parser_result_t parser_res = parse(scanner_res.tokens, scanner_res.token_count);
    arena_destroy(&scanner_res.arena);

    if (parser_debug_mode) {
        printf("Parser output debug start\n\n");
        print_statements(parser_res.stmts, parser_res.stmt_count);
        printf("\nParser output debug end\n\n");
    }
    if (parser_res.err_count > 0) {
        for (size_t i = 0; i < parser_res.err_count; i++) {
            print_fe_err(src_file, parser_res.errs[i]);
        }
        arena_destroy(&scanner_res.arena);
        exit(64);
    }

    const analyzer_result_t analyzer_res = analyze(parser_res.stmts, parser_res.stmt_count);
    arena_destroy(&parser_res.arena);

    if (analyzer_debug_mode) {
        printf("Analyzer output debug start\n\n");
        print_typed_stmts(analyzer_res.stmts, analyzer_res.stmt_count);
        printf("\nAnalyzer output debug end\n\n");
    }
    if (analyzer_res.err_count > 0) {
        for (size_t i = 0; i < analyzer_res.err_count; i++) {
            print_fe_err(src_file, analyzer_res.errs[i]);
        }
        arena_destroy(&analyzer.arena);
        exit(64);
    }

    arena_destroy(&analyzer.arena);
    free(src);
    return 0;
}
