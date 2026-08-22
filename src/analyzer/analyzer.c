#include "analyzer/analyzer.h"
#include "parser/parser.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

static analyzer_t analyzer;

static void analyzer_init();

static tstmt_t *stmt_analyze(const stmt_t *stmt);
static texpr_t *expr_analyze(const expr_t *expr);
static type_t *type_expr_analyze(const expr_t *expr);

static void declare_var(token_t name, type_t *type);
static void define_var(token_t name);
static void declare_fun(token_t name, tfunparam_avec_t *params, type_t *return_type);
static void declare_type(type_t *type);

static ascope_t *get_scope(ascope_kind_t scope_kind);
static ascope_t *get_first_scope(const ascope_kind_t scope_kinds[], size_t scope_count);
static obj_t *get_obj_span(span_t span);
static obj_t *get_obj(const char *name);

static void push_generic_scope(ascope_kind_t scope_kind);
static void push_fun_scope(type_t *return_type);
static void pop_scope(ascope_kind_t expected_scope);

analyzer_result_t analyze(stmt_avec_t *stmt_avec) {
    analyzer_init();
    tstmt_avec_t *tstmt_avec = tstmt_avec_make(analyzer.arena);

    AVEC_FOREACH(stmt, stmt_avec) {
        if (stmt->kind == STMT_FUN_DECLARATION) {
            tfunparam_avec_t *params = tfunparam_avec_make(analyzer.arena);
            AVEC_FOREACH(p, stmt->as.fun_decl.params) {
                tfunparam_avec_push(params, (tfunparam_t) {
                    .name = p.name,
                    .type = type_expr_analyze(p.type)
                });
            }
            type_t *return_type = type_expr_analyze(stmt->as.fun_decl.return_type);
            declare_fun(stmt->as.fun_decl.identifier, params, return_type);
        }
    }

    AVEC_FOREACH(stmt, stmt_avec) {
        tstmt_t *tstmt = stmt_analyze(stmt);
        if(tstmt != nullptr) tstmt_avec_push(tstmt_avec, tstmt);
    }

    obj_map_destroy(analyzer.scope->obj_map);
    free(analyzer.scope);

    return (analyzer_result_t) {
        .tstmt_avec = tstmt_avec,
        .err_avec = analyzer.err_avec,
        .arena = analyzer.arena
    };

}

static void analyzer_init() {
    arena_t *arena = arena_make(1 << 30u); 
    analyzer.arena = arena;

    analyzer.err_avec = err_avec_make(arena);
    analyzer.type_map = type_map_make();

    // Allocate on the heap rather than on the arena because scopes
    // will be popped and pushed frequently and are easy to clean
    analyzer.scope = malloc(sizeof(ascope_t));
    *analyzer.scope = (ascope_t){
        .obj_map = obj_map_make(),
        .kind = ASCOPE_GLOBAL,
    };

    for (size_t i = 0; i < sizeof prims / sizeof *prims; i++) {
        declare_type(prims[i]);
    }


    // FIXME: temp workaround to make print declaration work.
    // Maybe worth to actually put some effort to design better this part...
    static token_t print_name = {
        .span = {
            .src = "print",
            .length = 5,
            .start = 0,
            .end = 4
        },
        .kind = TOKEN_IDENTIFIER
    };
    tfunparam_avec_t *print_params = tfunparam_avec_make(analyzer.arena);  
    tfunparam_avec_push(print_params, (tfunparam_t) {
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
    });
    declare_fun(print_name, print_params, &ty_void);
}

static void make_err(span_t span, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    const int len = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    if (len < 0) return;

    char *buf = arena_alloc(analyzer.arena, (size_t)len + 1);
    if (!buf) return;

    va_start(args, fmt);
    vsnprintf(buf, (size_t)len + 1, fmt, args);
    va_end(args);

    err_avec_push(analyzer.err_avec, (fe_err_t){
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

static type_kind_t negatable_types[] = { TY_INT, TY_FLOAT };
static bool is_negatable(const type_t *type) {
    for (size_t i = 0; i < ARRAY_SIZE(negatable_types); i++) {
        if (type->kind == negatable_types[i]) {
            return true;
        }
    }
    return false;
}

tstmt_t *stmt_analyze(const stmt_t *stmt) {
    if (stmt == nullptr) return nullptr;

    tstmt_t *tstmt = arena_alloc(analyzer.arena, sizeof *tstmt);
    tstmt->span = stmt->span;

    switch (stmt->kind) {
        case STMT_VAR_DECLARATION: {
            tstmt->kind = STMT_VAR_DECLARATION;
            const token_t identifier = stmt->as.var_decl.identifier;
            tstmt->as.var_decl.identifier = identifier;

            type_t *decl_type = nullptr;
            if (stmt->as.var_decl.type != nullptr) {
                decl_type = type_expr_analyze(stmt->as.var_decl.type);
            }

            texpr_t *t_init_expr = expr_analyze(stmt->as.var_decl.initializer);
            type_t *inferred_type = nullptr;
            if (t_init_expr != nullptr) {
                inferred_type = t_init_expr->type;
            }
            tstmt->as.var_decl.initializer = t_init_expr;

            if (decl_type != nullptr && inferred_type != nullptr) {
                if (decl_type->kind == TY_ERROR || inferred_type->kind == TY_ERROR) {
                    tstmt->as.var_decl.type = &ty_error;
                    break;
                }
                if (decl_type == inferred_type) {
                    tstmt->as.var_decl.initializer->type = decl_type;
                    declare_var(identifier, decl_type);
                    define_var(identifier);
                    break;
                }
                // Here we are sure that the declaration type is different from the inferred type. Only allowed case
                // (for now) of type mismatch is when the initializer is a literal
                if (t_init_expr->kind == TEXPR_LITERAL && is_integral_type(decl_type) && is_integral_type(inferred_type)) {
                    tstmt->as.var_decl.initializer->type = decl_type;
                    declare_var(identifier, decl_type);
                    define_var(identifier);
                    break;
                }

                make_err(tstmt->span, 
                         "invalid variable declaration (mismatched type %s and type %s", 
                         decl_type->lexeme, inferred_type->lexeme);
                break;
            }
            if (decl_type != nullptr) {
                tstmt->as.var_decl.type = decl_type;
                declare_var(identifier, decl_type);
                break;
            }
            tstmt->as.var_decl.type = inferred_type;
            declare_var(identifier, inferred_type);
            define_var(identifier);
            break;
        }
        case STMT_FUN_DECLARATION: {
            obj_t *fun_obj = get_obj_span(stmt->as.fun_decl.identifier.span);

            type_t *fun_type = fun_obj->type;
            tfunparam_avec_t *params = fun_type->as.function.params;
            type_t *return_type = fun_type->as.function.return_type;

            push_fun_scope(return_type);

            size_t i = 0;
            AVEC_FOREACH(p, stmt->as.fun_decl.params) {
                const tfunparam_t param = tfunparam_avec_at(params, i);
                declare_var(param.name, param.type);
                define_var(param.name);
                i++;
            }

            tstmt_t *t_body = stmt_analyze(stmt->as.fun_decl.body);
            pop_scope(ASCOPE_FUN);

            *tstmt = (tstmt_t){
                .kind = STMT_FUN_DECLARATION,
                .as.fun_decl = {
                    .identifier = stmt->as.fun_decl.identifier,
                    .params = params,
                    .return_type = return_type,
                    .body = t_body
                }
            };
            break;
        }
        case STMT_BLOCK: {
            tstmt_avec_t *tstmts = tstmt_avec_make(analyzer.arena);

            AVEC_FOREACH(s, stmt->as.block) {
                tstmt_avec_push(tstmts, stmt_analyze(s));
            }

            *tstmt = (tstmt_t) {
                .kind = STMT_BLOCK,
                .as.block = tstmts
            };
            break;
        }
        case STMT_IF: {
            tstmt_t *head = tstmt;
            tstmt_t *current = tstmt;

            size_t i = 0;
            AVEC_FOREACH(condition, stmt->as._if.conditions) {
                stmt_t *then_body = stmt->as._if.then_branches->data[i]; 

                if(i == 0) {
                    *tstmt = (tstmt_t){
                        .kind = STMT_IF,
                        .span = then_body->span,
                        .as._if = {
                            .condition = expr_analyze(condition),
                            .then_branch = stmt_analyze(then_body)
                        }
                    };
                } else {
                    tstmt_t *else_branch = arena_alloc(analyzer.arena, sizeof *else_branch); 
                    *else_branch = (tstmt_t) {
                        .kind = STMT_IF,
                        .span = then_body->span,
                        .as._if = {
                            .condition = expr_analyze(condition),
                            .then_branch = stmt_analyze(then_body),
                        }
                    };
                    current->as._if.else_branch = else_branch;
                    current = else_branch;
                }
                
                i++;
            }

            // Just in case
            assert(head->kind == STMT_IF);
            assert(current->kind == STMT_IF);

            if(stmt->as._if.else_branch != nullptr) {
                tstmt_t *else_branch = stmt_analyze(stmt->as._if.else_branch);
                current->as._if.else_branch = else_branch;
            }
            break;
        }
        case STMT_FOR: {
            push_generic_scope(ASCOPE_FOR);
            *tstmt = (tstmt_t) {
                .kind = STMT_FOR,
                .as._for = {
                    .init = stmt_analyze(stmt->as._for.init),
                    .condition = expr_analyze(stmt->as._for.condition),
                    .increment = expr_analyze(stmt->as._for.increment),
                    .body = stmt_analyze(stmt->as._for.body)
                }
            };
            pop_scope(ASCOPE_FOR);
            break;
        }
        case STMT_WHILE: {
            push_generic_scope(ASCOPE_WHILE);
            *tstmt = (tstmt_t) {
                .kind = STMT_WHILE,
                .as._while = {
                    .body = stmt_analyze(stmt->as._while.body),
                    .condition = expr_analyze(stmt->as._while.condition)
                }
            };
            pop_scope(ASCOPE_WHILE);
            break;
        }
        case STMT_RETURN: {
            const ascope_t *scope = get_scope(ASCOPE_FUN);
            if (scope == nullptr) {
                make_err(stmt->span, "return statement used outside of function declaration");
                return nullptr;
            }

            texpr_t *t_expr = expr_analyze(stmt->as._return.value);
            // Do not check if the signature contains an error
            if (scope->return_type->kind != TY_ERROR && t_expr->type->kind != TY_ERROR && scope->return_type != t_expr->type) {
                make_err(stmt->span, "return value differs from function declaration. Expected %s, found %s",
                    scope->return_type->lexeme, t_expr->type->lexeme);
            }

            *tstmt = (tstmt_t) {
                .kind = STMT_RETURN,
                .span = stmt->span,
                .as._return = {
                    .value = t_expr
                }
            };
            break;
        }
        case STMT_CONTINUE: {
            tstmt->kind = STMT_CONTINUE;

            constexpr ascope_kind_t continue_scopes[] = {ASCOPE_WHILE, ASCOPE_FOR};
            const ascope_t *scope = get_first_scope(continue_scopes, ARRAY_SIZE(continue_scopes));
            if (!scope) {
                make_err(stmt->span, "cannot use continue outside loop");
                break;
            }
            break;
        }
        case STMT_BREAK: {
            tstmt->kind = STMT_BREAK;

            constexpr ascope_kind_t break_scopes[] = {ASCOPE_WHILE, ASCOPE_FOR};
            const ascope_t *scope = get_first_scope(break_scopes, ARRAY_SIZE(break_scopes));
            if (!scope) {
                make_err(stmt->span, "cannot use break outside loop");
                break;
            }
            break;
        }
        case STMT_EXPR: {
            tstmt->kind = stmt->kind;
            tstmt->as.expr = expr_analyze(stmt->as.expr);
            break;
        }
        default: return nullptr;
    }

    return tstmt;
}

texpr_t *expr_analyze(const expr_t *expr) {
    if (expr == nullptr) return nullptr;

    texpr_t *t_expr = arena_alloc(analyzer.arena, sizeof *t_expr);
    t_expr->span = expr->span;

    switch (expr->kind) {
        case EXPR_IDENTIFIER: {
            t_expr->kind = TEXPR_IDENTIFIER;
            t_expr->as.identifier = expr->as.identifier;

            const obj_t *obj = get_obj_span(expr->as.identifier.span);
            if (!obj) {
                make_err(expr->span, "cannot use undeclared identifier '%.*s'", expr->span.length, expr->span.src);
                t_expr->type = &ty_error;
            } else {
                t_expr->type = obj->type;
            }
            break;
        }
        case EXPR_LITERAL: {
            t_expr->kind = TEXPR_LITERAL;
            t_expr->as.literal = expr->as.literal;
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
            t_expr->kind = TEXPR_BINARY;
            t_expr->type = &ty_error;

            texpr_t *left = expr_analyze(expr->as.binary.left);
            texpr_t *right = expr_analyze(expr->as.binary.right);
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
            t_expr->kind = TEXPR_CALL;
            t_expr->type = &ty_error;

            texpr_avec_t *targs = texpr_avec_make(analyzer.arena);
            AVEC_FOREACH(targ, expr->as.call.args) {
                texpr_avec_push(targs, expr_analyze(targ));
            }

            texpr_t *typed_callee = expr_analyze(expr->as.call.callee);

            t_expr->as.call.callee = typed_callee;
            t_expr->as.call.args = targs;

            if (typed_callee == nullptr || typed_callee->type->kind == TY_ERROR) {
                break;
            }

            if (typed_callee->type->kind != TY_FUNCTION) {
                make_err(expr->as.call.callee->span, "cannot call non-function type %s",
                    typed_callee->type->lexeme);
                break;
            }

            size_t expected_args = typed_callee->type->as.function.params->size;
            size_t received_args = targs->size;
            if(expected_args != received_args) {
                make_err(expr->as.call.callee->span, "%.*s function requires %zu arguments but received %zu instead",
                         typed_callee->span.length, typed_callee->span.src, expected_args, received_args);
            }

            size_t i = 0;
            AVEC_FOREACH(targ, targs) {
                tfunparam_t param = tfunparam_avec_at(typed_callee->type->as.function.params, i);
                if(param.type != targ->type) {
                    make_err(targ->span, "cannot use \"%.*s\" (%s) as %s in argument to %.*s",
                        targ->span.length, targ->span.src, 
                        targ->type->lexeme, param.type->lexeme,
                        typed_callee->span.length, typed_callee->span.src);
                }
                i++;
            }

            t_expr->type = typed_callee->type->as.function.return_type;
            break;
        }
        case EXPR_UNARY: {
            t_expr->kind = TEXPR_UNARY;
            t_expr->span = expr->span;
            t_expr->as.unary.op = expr->as.unary.op;
            t_expr->type = &ty_error;

            texpr_t *typed_right = expr_analyze(expr->as.unary.right);
            t_expr->as.unary.right = typed_right;
            if (t_expr->as.unary.op.kind == TOKEN_MINUS) {
                if (typed_right->type->kind == TY_UINT) {
                    make_err(t_expr->span, "cannot apply minus (-) operator to uint expression");
                    break;
                }
                if (!is_negatable(typed_right->type)) {
                    make_err(t_expr->span, "cannot apply minus (-) operator to expression");
                    break;
                }
            } else if (t_expr->as.unary.op.kind == TOKEN_BANG) {
                if (typed_right->type->kind != TY_BOOL) {
                    make_err(t_expr->span, "cannot apply bang (!) operator to expression");
                    break;
                }
            } else {
                make_err(t_expr->as.unary.op.span, "invalid unary operator");
                break;
            }

            t_expr->type = typed_right->type;
            break;
        }
        case EXPR_GROUP: {
            t_expr->kind = TEXPR_GROUP;
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
            const obj_t *obj = get_obj_span(expr->span);
            if (obj == nullptr) goto error;
            if (obj->kind != OBJ_TYPE) {
                make_err(expr->span, "Not a type expression");
            }
            return obj->type;
        }
        // All of this switch case need to be filled with the respective type expressions.
        // For complex types, we will keep a map so that we instantiate only a singleton of that type
        // on demand, as discussed on top of the file
        default: break;
    }

error:
    make_err(expr->span, "Unknown type");
    return &ty_error;
}

static ascope_t *get_scope(const ascope_kind_t scope_kind) {
    ascope_t *scope = analyzer.scope;
    while (scope != nullptr) {
        if (scope->kind == scope_kind) return scope;
        scope = scope->parent;
    }
    return scope;
}

static ascope_t *get_first_scope(const ascope_kind_t scope_kinds[], const size_t scope_count) {
    ascope_t *scope = analyzer.scope;
    while (scope != nullptr) {
        for (size_t i = 0; i < scope_count; i++) {
            if (scope->kind == scope_kinds[i]) return scope;
        }
        scope = scope->parent;
    }
    return scope;
}

static bool obj_key(char *key, const size_t key_size, const char *name, const size_t name_len) {
    if (name_len > INT_MAX) return false;
    const int n = snprintf(key, key_size, "%.*s", (int)name_len, name);
    return n >= 0 && (size_t)n < key_size;
}

#define MAX_IDENT_LEN 255
#define OBJ_KEY_SIZE (sizeof("type:") + MAX_IDENT_LEN)

obj_t *get_obj_span(const span_t span) {
    char key[OBJ_KEY_SIZE];
    if (!obj_key(key, sizeof key, span.src, span.length)) return nullptr;

    for (const ascope_t *scope = analyzer.scope; scope != nullptr; scope = scope->parent) {
        obj_t *obj = obj_map_get(scope->obj_map, key);
        if (obj == nullptr) continue;
        return obj;
    }
    return nullptr;
}

obj_t *get_obj(const char *name) {
    return get_obj_span((span_t){ .src = name, .length = strlen(name) });
}

static void declare_fun(const token_t name, tfunparam_avec_t *params, type_t *return_type) {
    if (name.span.length > MAX_IDENT_LEN) {
        make_err(name.span, "identifier exceeds %d characters", MAX_IDENT_LEN);
        return;
    }

    char *obj_name = arena_alloc(analyzer.arena, name.span.length + 1);
    memcpy(obj_name, name.span.src, name.span.length);
    obj_name[name.span.length] = '\0';

    if (obj_map_get(analyzer.scope->obj_map, obj_name) != nullptr) {
        make_err(name.span, "redeclaration of function %.*s", (int)name.span.length, name.span.src);
        return;
    }

    type_t *fun_type = arena_alloc(analyzer.arena, sizeof *fun_type);
    *fun_type = (type_t) {
        .kind = TY_FUNCTION,
        .lexeme = obj_name,
        .as.function = {
            .params = params,
            .return_type = return_type
        }
    };

    obj_map_put(analyzer.scope->obj_map, obj_name, (obj_t){
        .kind = OBJ_FUNCTION,
        .name = obj_name,
        .type = fun_type,
        .defined = true
    });
}

void declare_var(const token_t name, type_t *type) {
    if (name.span.length > MAX_IDENT_LEN) {
        make_err(name.span, "identifier exceeds %d characters", MAX_IDENT_LEN);
        return;
    }

    char *obj_name = arena_alloc(analyzer.arena, name.span.length + 1);
    memcpy(obj_name, name.span.src, name.span.length);
    obj_name[name.span.length] = '\0';


    if (obj_map_get(analyzer.scope->obj_map, obj_name) != nullptr) {
        make_err(name.span, "redeclaration of variable %.*s", (int)name.span.length, name.span.src);
        return;
    }

    obj_map_put(analyzer.scope->obj_map, obj_name, (obj_t) {
        .kind = OBJ_VARIABLE,
        .name = obj_name,
        .type = type,
        .defined = false,
    });
}

void define_var(const token_t name) {
    obj_t *obj = get_obj_span(name.span);
    if (!obj) {
        make_err(name.span, "cannot define undeclared variable %.*s",
                 (int)name.span.length, name.span.src);
        return;
    }
    if (obj->kind != OBJ_VARIABLE) {
        make_err(name.span, "can only define variables");
    }
    obj->defined = true;
}

void declare_type(type_t *type) {
    obj_map_put(analyzer.scope->obj_map, type->lexeme, (obj_t) {
        .kind = OBJ_TYPE,
        .name = type->lexeme,
        .defined = true,
        .type = type,
    });
}

void push_generic_scope(const ascope_kind_t scope_kind) {
    ascope_t *new_scope = malloc(sizeof *new_scope);
    *new_scope = (ascope_t) {
        .kind = scope_kind,
        .parent = analyzer.scope,
        .obj_map = obj_map_make(),
    };
    analyzer.scope = new_scope;
}

void push_fun_scope(type_t *return_type) {
    ascope_t *new_scope = malloc(sizeof *new_scope);
    *new_scope = (ascope_t) {
        .kind = ASCOPE_FUN,
        .parent = analyzer.scope,
        .obj_map = obj_map_make(),
        .return_type = return_type
    };
    analyzer.scope = new_scope;
}

void pop_scope(ascope_kind_t expected_scope) {
    if (analyzer.scope == nullptr) {
        fprintf(stderr, "no scopes left to pop");
        exit(EINVAL);
    }
    if (analyzer.scope->kind != expected_scope) {
        fprintf(stderr, "unexpected scope to pop.");
    }
    ascope_t *parent = analyzer.scope->parent;
    obj_map_destroy(analyzer.scope->obj_map);
    analyzer.scope = parent;
}
