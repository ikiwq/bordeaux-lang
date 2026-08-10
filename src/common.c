#include "common.h"

typedef struct {
    size_t line;
    size_t col;
} line_col_t;

line_col_t lc_locate(const src_file_t src_file, const size_t span_start) {
    size_t low = 0;
    size_t high = src_file.line_vec.size;

    while (low < high) {
        const size_t mid = low + (high - low) / 2;
        if (src_file.line_vec.data[mid] <= span_start)
            low = mid + 1;
        else
            high = mid;
    }

    const size_t idx = low - 1;
    return (line_col_t) {
        .line = idx + 1,
        .col  = span_start - src_file.line_vec.data[idx] + 1,
    };
}

void print_fe_err(const src_file_t src_file, const fe_err_t fe_err) {
    const line_col_t lc = lc_locate(src_file, fe_err.span.start);

    const size_t line_start = src_file.line_vec.data[lc.line - 1];
    size_t line_end;
    if (lc.line < src_file.line_vec.size)
        line_end = src_file.line_vec.data[lc.line] - 1;
    else
        line_end = src_file.len;
    if (line_end > src_file.len) line_end = src_file.len;

    const char  *text = src_file.src + line_start;
    const size_t t_len = line_end - line_start;

    fprintf(stderr, "%s:%zu:%zu: error: %s\n",
            src_file.path, lc.line, lc.col, fe_err.err);
    fprintf(stderr, "%5zu | %.*s\n", lc.line, (int)t_len, text);
    fprintf(stderr, "      | ");

    for (size_t i = 0; i + 1 < lc.col; i++)
        fputc(text[i] == '\t' ? '\t' : ' ', stderr);

    size_t caret_end = fe_err.span.end;
    if (caret_end > line_end) caret_end = line_end;
    const size_t n = caret_end > fe_err.span.start ? caret_end - fe_err.span.start : 1;

    for (size_t i = 0; i < n; i++) fputc('^', stderr);
    fputc('\n', stderr);
}
