#ifndef ASM_EMITTER_H
#define ASM_EMITTER_H

#include "analyzer/statement.h"

#define STRMAP_TYPE size_t
#define STRMAP_NAME local
#include "dsa/strmap.h"

typedef struct asmscope asmscope_t;

struct asmscope {
    local_map_t *locals;
    size_t next_local_off; // next local offset

    asmscope_t *parent;
};

typedef struct {
    FILE *output;
    asmscope_t *scope;
} emitter_t;

void emit_asm(const char* filename, tstmt_avec_t *tstmt_avec);

#endif // ASM_EMITTER_H
