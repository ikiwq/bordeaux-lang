#include "types.h"

#include <stdio.h>

type_t ty_int     = { .lexeme = STR_LIT("int"),     .kind = TY_INT    };
type_t ty_float   = { .lexeme = STR_LIT("float"),   .kind = TY_FLOAT  };
type_t ty_string  = { .lexeme = STR_LIT("string"),  .kind = TY_STRING };
type_t ty_bool    = { .lexeme = STR_LIT("bool"),    .kind = TY_BOOL   };
type_t ty_char    = { .lexeme = STR_LIT("char"),    .kind = TY_CHAR   };
type_t ty_void    = { .lexeme = STR_LIT("void"),    .kind = TY_VOID   };
type_t ty_error   = { .lexeme = STR_LIT("error"),   .kind = TY_ERROR  };

type_t *const prims[] = {
    &ty_int,
    &ty_float,
    &ty_string,
    &ty_bool,
    &ty_char,
    &ty_void,
    &ty_error,
};

const size_t prims_count = ARRAY_SIZE(prims);

size_t get_type_size(type_t *type) {
    switch(type->kind) {
        case TY_VOID: return 0;
        case TY_ERROR: return 0;
        case TY_BOOL: return 1;
        case TY_CHAR: return 8;
        case TY_FLOAT: return 8;
        case TY_INT: return 8;
        case TY_STRING: return 16;
        default: return 0;
    }
}
