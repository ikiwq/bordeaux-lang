#ifndef BORDEAUX_SCANNER_H
#define BORDEAUX_SCANNER_H

#include "scanner/token.h"

typedef struct {
    const char *src;
    const char *start;
    const char *current;
} scanner_t;

void scanner_init(const char *src);

token_t scan_token(void);

#endif // BORDEAUX_SCANNER_H
