#include "common.h"

#include <stdarg.h>
#include <stdio.h>

static src_file_t file;

void errors_init(const src_file_t src_file) {
    file = src_file;
}

void report_err(const span_t span, const char *fmt, ...) {
    const size_t start = span.start > file.len ? file.len : span.start;

    size_t line = 1;
    size_t line_start = 0;
    for (size_t i = 0; i < start; i++) {
        if (file.src[i] == '\n') {
            line++;
            line_start = i + 1;
        }
    }

    size_t line_end = line_start;
    while (line_end < file.len && file.src[line_end] != '\n') line_end++;

    const size_t col = start - line_start + 1;
    const char *text = file.src + line_start;

    fprintf(stderr, "%s:%zu:%zu: error: ", file.path, line, col);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);

    fprintf(stderr, "%5zu | %.*s\n", line, (int)(line_end - line_start), text);
    fprintf(stderr, "      | ");

    for (size_t i = 0; i + 1 < col; i++)
        fputc(text[i] == '\t' ? '\t' : ' ', stderr);

    size_t caret_end = span.end;
    if (caret_end > line_end) caret_end = line_end;
    const size_t n = caret_end > start ? caret_end - start : 1;

    for (size_t i = 0; i < n; i++) fputc('^', stderr);
    fputc('\n', stderr);
}
