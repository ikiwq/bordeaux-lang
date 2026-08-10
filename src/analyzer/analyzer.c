#include "analyzer/analyzer.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

static void analyzer_init();

static typed_stmt_t *stmt_analyze(const stmt_t *stmt);
static typed_expr_t *expr_analyze(const expr_t *expr);
static type_t *type_expr_analyze(const expr_t *expr);

static void declare_var(token_t name, type_t *type);
static void define_var(token_t name);
void declare_fun(token_t name, typed_fun_param_t *params, size_t param_count, type_t *return_type);
static void declare_type(type_t *type);

static scope_t *get_scope(scope_kind_t kind);
static scope_t *get_first_scope(const scope_kind_t scope_kinds[], size_t scope_count);
static obj_t *get_obj_span(obj_kind_t kind, span_t span);
static obj_t *get_obj(obj_kind_t kind, const char *name);

static void push_generic_scope(scope_kind_t scope_kind);
static void push_fun_scope(type_t *return_type);
static void pop_scope(scope_kind_t expected_scope);

analyzer_result_t analyze(stmt_t **statements, const size_t statement_count) {
    analyzer_init();
    t_stmt_vec_t t_stmt_vec = t_stmt_vec_init();

    for (size_t i = 0; i < statement_count; i++) {
        typed_stmt_t *typed_stmt = stmt_analyze(statements[i]);
        if (typed_stmt != nullptr) t_stmt_vec_push(&t_stmt_vec, typed_stmt);
    }

    const size_t err_count = analyzer.err_vec.size;
    fe_err_t *errs =
        arena_copy(&analyzer.arena, analyzer.err_vec.data, err_count * sizeof *errs);
    err_vec_destroy(&analyzer.err_vec);

    const size_t stmt_count = t_stmt_vec.size;
    typed_stmt_t **stmts =
        arena_copy(&analyzer.arena, t_stmt_vec.data, stmt_count * sizeof *stmts);
    t_stmt_vec_destroy(&t_stmt_vec);

    return (analyzer_result_t) {
        .stmt_count = stmt_count,
        .stmts = stmts,
        .err_count = err_count,
        .errs = errs
    };
}

// A couple of words on ...this. Type checking is considerably hard if done in a not
// so good way. Suppose for example that you have this sequence of expressions:
// let a: int64 = 5;
// let b: int64 = 10;
// let c = a + b;
// When comparing the type expressions of a and b you could recursively check the types...
// or simply have a singleton of every type. So, each type would definitively have one and only memory address.
// At this point, comparing two types becomes trivial: just compare the two memory addresses.
// For type contructors (e.g. arrays, pointers, references), it's trickier because there are
// an infinite amount of combinations that we can't pre-allocate.
// We can instead have a hashmap where we dynamically put new type constructors and then retrieve it when necessary.
// The latter won't depend on the scope, since a type constructor relies on type expressions, which are already interned in the
// corresponding scope
static type_t ty_int8    = { .lexeme = "int8",    .kind = TY_INT,   .as._int.kind   = INT8    };
static type_t ty_int16   = { .lexeme = "int16",   .kind = TY_INT,   .as._int.kind   = INT16   };
static type_t ty_int32   = { .lexeme = "int32",   .kind = TY_INT,   .as._int.kind   = INT32   };
static type_t ty_int64   = { .lexeme = "int64",   .kind = TY_INT,   .as._int.kind   = INT64   };
static type_t ty_uint8   = { .lexeme = "uint8",   .kind = TY_UINT,  .as._int.kind   = INT8    };
static type_t ty_uint16  = { .lexeme = "uint16",  .kind = TY_UINT,  .as._int.kind   = INT16   };
static type_t ty_uint32  = { .lexeme = "uint32",  .kind = TY_UINT,  .as._int.kind   = INT32   };
static type_t ty_uint64  = { .lexeme = "uint64",  .kind = TY_UINT,  .as._int.kind   = INT64   };
static type_t ty_float32 = { .lexeme = "float32", .kind = TY_FLOAT, .as._float.kind = FLOAT32 };
static type_t ty_float64 = { .lexeme = "float64", .kind = TY_FLOAT, .as._float.kind = FLOAT64 };
static type_t ty_string  = { .lexeme = "string",  .kind = TY_STRING };
static type_t ty_bool    = { .lexeme = "bool",    .kind = TY_BOOL  };
static type_t ty_char    = { .lexeme = "char",    .kind = TY_CHAR  };
static type_t ty_void    = { .lexeme = "void",    .kind = TY_VOID  };
static type_t ty_error   = { .lexeme = "error",   .kind = TY_ERROR };

static type_t *const prims[] = {
    &ty_int8,
    &ty_int16,
    &ty_int32,
    &ty_int64,
    &ty_uint8,
    &ty_uint16,
    &ty_uint32,
    &ty_uint64,
    &ty_float32,
    &ty_float64,
    &ty_string,
    &ty_bool,
    &ty_char,
    &ty_void,
    &ty_error,
};

static token_t print_name = {
    .span = {
        .src = "print",
        .length = 5,
        .start = 0,
        .end = 4
    },
    .kind = TOKEN_IDENTIFIER
};
static typed_fun_param_t print_arg0 = {
    .name = {
        .span = {
            .src = "arg0",
            .length = 4,
            .start = 0,
            .end = 3
        },
        .kind = TOKEN_IDENTIFIER
    },
    .type = &ty_string
};
static typed_fun_param_t print_params[1];

static void analyzer_init() {
    analyzer.err_vec = err_vec_init();
    analyzer.arena = arena_make(1 << 28u); // 256 MB
    analyzer.type_map = type_map_init();

    analyzer.scope = arena_alloc(&analyzer.arena, sizeof(scope_t));
    *analyzer.scope = (scope_t){
        .obj_map = obj_map_init(),
        .kind = SCOPE_GLOBAL,
    };

    for (size_t i = 0; i < sizeof prims / sizeof *prims; i++) {
        declare_type(prims[i]);
    }

    // FIXME: temp workaround to make function declaration work.
    // Maybe worth to actually put some effort to design better this part...
    print_params[0] = print_arg0;
    declare_fun(print_name, print_params, 1, &ty_void);
}

static void make_err(span_t span, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    const int len = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    if (len < 0) return;

    char *buf = arena_alloc(&analyzer.arena, (size_t)len + 1);
    if (!buf) return;

    va_start(args, fmt);
    vsnprintf(buf, (size_t)len + 1, fmt, args);
    va_end(args);

    err_vec_push(&analyzer.err_vec, (fe_err_t){
        .span = span,
        .err = buf
    });
}


static type_kind_t comparable_types[] = { TY_INT, TY_UINT, TY_FLOAT, TY_CHAR, TY_STRING };
static bool is_comparable(const type_t *type) {
    for (size_t i = 0; i < ARRAY_SIZE(comparable_types); i++) {
        if (type->kind == comparable_types[i]) {
            return true;
        }
    }
    return false;
}

static type_kind_t computable_types[] = { TY_INT, TY_UINT, TY_FLOAT };
static bool is_computable(const type_t *type) {
    for (size_t i = 0; i < ARRAY_SIZE(computable_types); i++) {
        if (type->kind == computable_types[i]) {
            return true;
        }
    }
    return false;
}

typed_stmt_t *stmt_analyze(const stmt_t *stmt) {
    if (stmt == nullptr) return nullptr;

    typed_stmt_t *t_stmt = arena_alloc(&analyzer.arena, sizeof *t_stmt);
    t_stmt->span = stmt->span;

    switch (stmt->kind) {
        case STMT_VAR_DECLARATION: {
            t_stmt->kind = STMT_VAR_DECLARATION;
            const token_t identifier = stmt->as.var_decl.identifier;
            t_stmt->as.var_decl.identifier = identifier;

            type_t *decl_type = nullptr;
            if (stmt->as.var_decl.type != nullptr) {
                decl_type = type_expr_analyze(stmt->as.var_decl.type);
            }

            typed_expr_t *t_init_expr = expr_analyze(stmt->as.var_decl.initializer);
            type_t *inferred_type = nullptr;
            if (t_init_expr != nullptr) {
                inferred_type = t_init_expr->type;
            }
            t_stmt->as.var_decl.initializer = t_init_expr;

            if (decl_type != nullptr && inferred_type != nullptr) {
                if (decl_type->kind == TY_ERROR || inferred_type->kind == TY_ERROR) {
                    t_stmt->as.var_decl.type = &ty_error;
                    break;
                }
                if (decl_type == inferred_type) {
                    t_stmt->as.var_decl.initializer->type = decl_type;
                    declare_var(identifier, decl_type);
                    define_var(identifier);
                    break;
                }
                // Here we are sure that the declaration type is different from the inferred type. Only allowed case
                // (for now) of type mismatch is when the initializer is a literal
                if (t_init_expr->kind == TY_EXPR_LITERAL && is_integral_type(decl_type) && is_integral_type(inferred_type)) {
                    t_stmt->as.var_decl.initializer->type = decl_type;
                    declare_var(identifier, decl_type);
                    define_var(identifier);
                    break;
                }

                make_err(t_stmt->span, "invalid variable declaration (mismatched type %s and type %s",
                    decl_type->lexeme, inferred_type->lexeme);
                break;
            }
            if (decl_type != nullptr) {
                t_stmt->as.var_decl.type = decl_type;
                declare_var(identifier, decl_type);
                break;
            }
            t_stmt->as.var_decl.type = inferred_type;
            declare_var(identifier, inferred_type);
            define_var(identifier);
            break;
        }
        case STMT_FUN_DECLARATION: {
            const size_t params_count = stmt->as.fun_decl.params_count;
            typed_fun_param_t *params = nullptr;
            if (params_count > 0) {
                params = arena_alloc(&analyzer.arena, params_count * sizeof *params);
                for (size_t i = 0; i < stmt->as.fun_decl.params_count; i++) {
                    params[i] = (typed_fun_param_t) {
                        .name = stmt->as.fun_decl.params[i].name,
                        .type = type_expr_analyze(stmt->as.fun_decl.params[i].type),
                    };
                }
            }

            type_t *return_type = type_expr_analyze(stmt->as.fun_decl.return_type);
            declare_fun(stmt->as.fun_decl.identifier, params, params_count, return_type);

            push_fun_scope(return_type);
            for (size_t i = 0; i < params_count; i++) {
                const typed_fun_param_t param = params[i];
                declare_var(param.name, param.type);
                define_var(param.name);
            }

            typed_stmt_t *t_body = stmt_analyze(stmt->as.fun_decl.body);
            pop_scope(SCOPE_FUN);

            *t_stmt = (typed_stmt_t){
                .kind = STMT_FUN_DECLARATION,
                .as.fun_decl = {
                    .identifier = stmt->as.fun_decl.identifier,
                    .params = params,
                    .param_count = params_count,
                    .return_type = return_type,
                    .body = t_body
                }
            };
            break;
        }
        case STMT_BLOCK: {
            const size_t stmt_count = stmt->as.block.stmt_count;
            typed_stmt_t **t_stmts = nullptr;
            if (stmt_count > 0) {
                t_stmts = arena_alloc(&analyzer.arena, stmt_count * sizeof *t_stmts);
                for (size_t i = 0; i < stmt_count; i++) {
                    t_stmts[i] = stmt_analyze(stmt->as.block.stmts[i]);
                }
            }

            *t_stmt = (typed_stmt_t) {
                .kind = STMT_BLOCK,
                .as.block = {
                    .stmt_count = stmt_count,
                    .stmts = t_stmts,
                }
            };
            break;
        }
        case STMT_IF: {
            *t_stmt = (typed_stmt_t) {
                .kind = STMT_IF,
                .as._if = {
                    .condition = expr_analyze(stmt->as._if.condition),
                    .then_branch = stmt_analyze(stmt->as._if.then_branch),
                    .else_branch = stmt_analyze(stmt->as._if.else_branch)
                }
            };
            break;
        }
        case STMT_FOR: {
            push_generic_scope(SCOPE_FOR);
            *t_stmt = (typed_stmt_t) {
                .kind = STMT_FOR,
                .as._for = {
                    .init = stmt_analyze(stmt->as._for.init),
                    .condition = expr_analyze(stmt->as._for.condition),
                    .increment = expr_analyze(stmt->as._for.increment),
                    .body = stmt_analyze(stmt->as._for.body)
                }
            };
            pop_scope(SCOPE_FOR);
            break;
        }
        case STMT_WHILE: {
            push_generic_scope(SCOPE_WHILE);
            *t_stmt = (typed_stmt_t) {
                .kind = STMT_WHILE,
                .as._while = {
                    .body = stmt_analyze(stmt->as._while.body),
                    .condition = expr_analyze(stmt->as._while.condition)
                }
            };
            pop_scope(SCOPE_WHILE);
            break;
        }
        case STMT_RETURN: {
            const scope_t *scope = get_scope(SCOPE_FUN);
            if (scope == nullptr) {
                make_err(stmt->span, "return statement used outside of function declaration");
                return nullptr;
            }

            typed_expr_t *t_expr = expr_analyze(stmt->as._return.value);
            // Do not check if the signature contains an error
            if (scope->return_type->kind != TY_ERROR && t_expr->type->kind != TY_ERROR && scope->return_type != t_expr->type) {
                make_err(stmt->span, "return value differs from function declaration. Expected %s, found %s",
                    scope->return_type->lexeme, t_expr->type->lexeme);
            }

            *t_stmt = (typed_stmt_t) {
                .kind = STMT_RETURN,
                .span = stmt->span,
                .as._return = {
                    .value = t_expr
                }
            };
            break;
        }
        case STMT_CONTINUE: {
            constexpr scope_kind_t continue_scopes[] = {SCOPE_WHILE, SCOPE_FOR};
            const scope_t *scope = get_first_scope(continue_scopes, ARRAY_SIZE(continue_scopes));
            if (!scope) {
                make_err(stmt->span, "cannot use continue outside loop");
                break;
            }
            break;
        }
        case STMT_BREAK: {
            constexpr scope_kind_t break_scopes[] = {SCOPE_WHILE, SCOPE_FOR};
            const scope_t *scope = get_first_scope(break_scopes, ARRAY_SIZE(break_scopes));
            if (!scope) {
                make_err(stmt->span, "cannot use break outside loop");
                break;
            }
            break;
        }
        case STMT_EXPR: {
            t_stmt->as.expr = expr_analyze(stmt->as.expr);
            break;
        }
        default: return nullptr;
    }

    return t_stmt;
}

typed_expr_t *expr_analyze(const expr_t *expr) {
    if (expr == nullptr) return nullptr;

    typed_expr_t *t_expr = arena_alloc(&analyzer.arena, sizeof(typed_stmt_t));
    t_expr->span = expr->span;

    switch (expr->kind) {
        case EXPR_IDENTIFIER: {
            t_expr->kind = TY_EXPR_IDENTIFIER;
            t_expr->as.identifier = expr->as.identifier;

            const obj_t *var_identifier = get_obj_span(OBJ_VARIABLE, expr->as.identifier.span);
            if (!var_identifier) {
                make_err(expr->span, "cannot use undeclared identifier %.*s", expr->span.length, expr->span.src);
                t_expr->type = &ty_error;
            } else {
                t_expr->type = var_identifier->as.variable.type ;
            }
            break;
        }
        case EXPR_LITERAL: {
            t_expr->kind = TY_EXPR_LITERAL;
            switch (expr->as.literal.kind) {
                case LIT_BOOL: t_expr->type = &ty_bool; break;
                case LIT_STRING: t_expr->type = &ty_string; break;
                case LIT_CHAR: t_expr->type = &ty_char; break;
                case LIT_INTEGER: t_expr->type = &ty_int32; break;
                case LIT_FLOAT: t_expr->type = &ty_float32; break;
            }
            break;
        }
        case EXPR_BINARY: {
            t_expr->kind = TY_EXPR_BINARY;
            t_expr->type = &ty_error;

            typed_expr_t *left = expr_analyze(expr->as.binary.left);
            typed_expr_t *right = expr_analyze(expr->as.binary.right);
            t_expr->as.binary.left = left;
            t_expr->as.binary.op = expr->as.binary.op;
            t_expr->as.binary.right = right;

            if (left == nullptr || right == nullptr ||
                left->type->kind == TY_ERROR || right->type->kind == TY_ERROR) {
                // If the expressions didn't compute, just skip the type checking
                break;
            }

            if (left->type != right->type) {
                make_err(expr->span, "invalid operation (mismatched type %s and %s)",
                    left->type->lexeme, right->type->lexeme);
                break;
            }

            if (is_comparing_token(expr->as.binary.op.kind)) {
                if (!is_comparable(left->type)) {
                    make_err(expr->span, "invalid operation (cannot compare type %s)", left->type->lexeme);
                    break;
                }
                t_expr->type = &ty_bool;
                break;
            }
            if (is_computing_token(expr->as.binary.op.kind)) {
                if (left->type->kind == TY_STRING && expr->as.binary.op.kind == TOKEN_PLUS) {
                    t_expr->type = left->type;
                    break;
                }
                if (!is_computable(left->type)) {
                    make_err(expr->span, "invalid operation (cannot compute type %s)", left->type->lexeme);
                    break;
                }
                t_expr->type = left->type;
                break;
            }
            if (is_logical_token(expr->as.binary.op.kind)) {
                if (left->type != &ty_bool) {
                    make_err(expr->span, "invalid operation (cannot compute type %s)", left->type->lexeme);
                    break;
                }
                t_expr->type = left->type;
                break;
            }

            t_expr->type = left->type;
            break;
        }
        case EXPR_CALL: {
            t_expr->kind = TY_EXPR_CALL;
            t_expr->type = &ty_error;

            const size_t args_count = expr->as.call.args_count;
            typed_expr_t **typed_args = nullptr;
            if (args_count > 0) {
                typed_args = arena_alloc(&analyzer.arena, args_count * sizeof *typed_args);
                for (size_t i = 0; i < args_count; i++) {
                    typed_args[i] = expr_analyze(expr->as.call.args[i]);
                }
            }

            if (expr->as.call.callee->kind == EXPR_IDENTIFIER) {
                const obj_t *fun_obj = get_obj_span(OBJ_FUNCTION, expr->as.call.callee->span);
                if (fun_obj == nullptr) {
                    make_err(expr->as.call.callee->span, "cannot call undeclared function %.*s",
                        expr->as.call.callee->span.length, expr->as.call.callee->span.src);
                    break;
                }

                t_expr->as.call.args_count = args_count;
                t_expr->as.call.args = typed_args;
                t_expr->type = fun_obj->as.function.return_type;
            } else {
                // TODO: Temporary workaround
                fprintf(stderr, "returning functions from a function is not yet supported");
                exit(EINVAL);
            }

            break;
        }
        case EXPR_GROUP: {
            t_expr->kind = TY_EXPR_GROUP;
            t_expr->as.group.inner = expr_analyze(expr->as.group.inner);
            t_expr->type = t_expr->as.group.inner->type;
            break;
        }
        default: return nullptr;
    }

    return t_expr;
}

type_t *type_expr_analyze(const expr_t *expr) {
    if (expr == nullptr) return nullptr;

    switch (expr->kind) {
        case EXPR_TYPE_NAME: {
            const obj_t *obj = get_obj_span(OBJ_TYPE, expr->span);
            if (obj == nullptr) goto error;
            return obj->as.type;
        }
        // All of this switch case need to be filled with the respective type expressions.
        // For complex types, we will keep a map so that we instantiate only a singleton of that type
        // on demand, as discussed on top of the file
        default: break;
    }

error:
    make_err(expr->span, "Unknown type");
    return get_obj(OBJ_TYPE, "error")->as.type;
}

static scope_t *get_scope(const scope_kind_t scope_kind) {
    scope_t *scope = analyzer.scope;
    while (scope != nullptr) {
        if (scope->kind == scope_kind) return scope;
        scope = scope->parent;
    }
    return scope;
}

static scope_t *get_first_scope(const scope_kind_t scope_kinds[], const size_t scope_count) {
    scope_t *scope = analyzer.scope;
    while (scope != nullptr) {
        for (size_t i = 0; i < scope_count; i++) {
            if (scope->kind == scope_kinds[i]) return scope;
        }
        scope = scope->parent;
    }
    return scope;
}

static const char *obj_kind_prefix(const obj_kind_t kind) {
    switch (kind) {
        case OBJ_TYPE:     return "type";
        case OBJ_VARIABLE: return "var";
        case OBJ_FUNCTION: return "fun";
    }
    assert(!"unknown obj_kind_t");
}

static bool obj_key(char *key, const size_t key_size, const obj_kind_t kind, const char *name, const size_t name_len) {
    if (name_len > INT_MAX) return false;
    const int n = snprintf(key, key_size, "%s:%.*s", obj_kind_prefix(kind), (int)name_len, name);
    return n >= 0 && (size_t)n < key_size;
}

#define MAX_IDENT_LEN 255
#define OBJ_KEY_SIZE (sizeof("type:") + MAX_IDENT_LEN)

obj_t *get_obj_span(const obj_kind_t kind, const span_t span) {
    char key[OBJ_KEY_SIZE];
    if (!obj_key(key, sizeof key, kind, span.src, span.length)) return nullptr;

    for (const scope_t *scope = analyzer.scope; scope != nullptr; scope = scope->parent) {
        obj_t *obj = obj_map_get(&scope->obj_map, key);
        if (obj == nullptr) continue;
        assert(obj->kind == kind);
        return obj;
    }
    return nullptr;
}

obj_t *get_obj(const obj_kind_t kind, const char *name) {
    return get_obj_span(kind, (span_t){ .src = name, .length = strlen(name) });
}

void declare_fun(const token_t name, typed_fun_param_t *params, const size_t param_count, type_t *return_type) {
    if (name.span.length > MAX_IDENT_LEN) {
        make_err(name.span, "identifier exceeds %d characters", MAX_IDENT_LEN);
        return;
    }

    const char *prefix = obj_kind_prefix(OBJ_FUNCTION);
    const size_t prefix_len = strlen(prefix);
    const size_t total = prefix_len + 1 + name.span.length + 1;

    char *key = arena_alloc(&analyzer.arena, total);
    memcpy(key, prefix, prefix_len);
    key[prefix_len] = ':';
    char *obj_name = key + prefix_len + 1;
    memcpy(obj_name, name.span.src, name.span.length);
    obj_name[name.span.length] = '\0';

    if (obj_map_get(&analyzer.scope->obj_map, key) != nullptr) {
        make_err(name.span, "redeclaration of function %.*s", (int)name.span.length, name.span.src);
        return;
    }

    obj_map_put(&analyzer.scope->obj_map, key, (obj_t){
        .kind = OBJ_FUNCTION,
        .name = obj_name,
        .as.function = {
            .params = params,
            .param_count = param_count,
            .return_type = return_type
        }
    });
}

void declare_var(const token_t name, type_t *type) {
    if (name.span.length > MAX_IDENT_LEN) {
        make_err(name.span, "identifier exceeds %d characters", MAX_IDENT_LEN);
        return;
    }

    const char *prefix = obj_kind_prefix(OBJ_VARIABLE);
    const size_t prefix_len = strlen(prefix);
    const size_t total = prefix_len + 1 + name.span.length + 1;

    char *key = arena_alloc(&analyzer.arena, total);
    memcpy(key, prefix, prefix_len);
    key[prefix_len] = ':';
    char *obj_name = key + prefix_len + 1;
    memcpy(obj_name, name.span.src, name.span.length);
    obj_name[name.span.length] = '\0';

    if (obj_map_get(&analyzer.scope->obj_map, key) != nullptr) {
        make_err(name.span, "redeclaration of variable %.*s", (int)name.span.length, name.span.src);
        return;
    }

    obj_map_put(&analyzer.scope->obj_map, key, (obj_t) {
        .kind = OBJ_VARIABLE,
        .name = obj_name,
        .as.variable = { .type = type, .defined = false },
    });
}

void define_var(const token_t name) {
    obj_t *obj = get_obj_span(OBJ_VARIABLE, name.span);
    if (!obj) {
        make_err(name.span, "cannot define undeclared variable %.*s",
                 (int)name.span.length, name.span.src);
        return;
    }
    obj->as.variable.defined = true;
}

void declare_type(type_t *type) {
    const size_t len = strlen(type->lexeme);
    const size_t total = sizeof("type:") + len;
    char *key = arena_alloc(&analyzer.arena, total);
    snprintf(key, total, "type:%s", type->lexeme);

    obj_map_put(&analyzer.scope->obj_map, key, (obj_t) {
        .kind = OBJ_TYPE,
        .name = type->lexeme,
        .as.type = type,
    });
}

// NOTE FOR SCOPES: Only the global scope will be instantiated into the arena since it will be persisted
// after the analysis. Every other scope is temporary
void push_generic_scope(scope_kind_t scope_kind) {
    scope_t *new_scope = malloc(sizeof(scope_t));
    *new_scope = (scope_t) {
        .kind = scope_kind,
        .parent = analyzer.scope,
        .obj_map = obj_map_init(),
    };
    analyzer.scope = new_scope;
}

void push_fun_scope(type_t *return_type) {
    scope_t *new_scope = malloc(sizeof(scope_t));
    *new_scope = (scope_t) {
        .kind = SCOPE_FUN,
        .parent = analyzer.scope,
        .obj_map = obj_map_init(),
        .return_type = return_type
    };
    analyzer.scope = new_scope;
}

void pop_scope(scope_kind_t expected_scope) {
    if (analyzer.scope == nullptr) {
        fprintf(stderr, "no scopes left to pop");
        exit(EINVAL);
    }
    if (analyzer.scope->kind != expected_scope) {
        fprintf(stderr, "unexpected scope to pop.");
    }
    scope_t *parent = analyzer.scope->parent;
    obj_map_destroy(&analyzer.scope->obj_map);
    analyzer.scope = parent;
}
