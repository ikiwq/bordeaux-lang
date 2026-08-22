#include "asm/emitter.h"
#include "parser/parser.h"

#include <assert.h>
#include <stdarg.h>

static emitter_t emitter;

static void emit_tstmt(tstmt_t*);
static void emit_texpr(texpr_t*);

static void scope_push();
static void scope_put(const char*, size_t);
static void scope_pop();

static void emit_string(const char*, ...);
static void emit_label(const char*, ...);
static void emit_instr(const char*, ...);
static void emit_newline(size_t);

static void get_identifier(char *, size_t, span_t);

void emit_asm(const char *filename, tstmt_avec_t *tstmt_avec) {
    emitter = (emitter_t) {
        .output = fopen(filename, "w+"),
        .scope = nullptr
    };
    assert(emitter.output != nullptr);

    AVEC_FOREACH(tstmt, tstmt_avec) {
        emit_tstmt(tstmt);
    }

    fclose(emitter.output);
    emitter.output = nullptr;
}

static void emit_tstmt(tstmt_t *tstmt) {
    switch(tstmt->kind) {
        case STMT_FUN_DECLARATION: {
            char identifier[256];
            get_identifier(identifier, sizeof identifier, tstmt->as.fun_decl.identifier.span);

            if(strcmp(identifier, "main") == 0) {
                emit_string(".global main");
            }

            emit_label(identifier);
            scope_push();

            size_t i = 0;
            AVEC_FOREACH(param, tstmt->as.fun_decl.params) {
                char param_name[256];
                get_identifier(param_name, sizeof param_name, param.name.span);
                scope_put(param_name, 4 * i);

                i++;
            }
            emitter.scope->next_local_off = (i + 1) * 4;

            emit_tstmt(tstmt->as.fun_decl.body);

            scope_pop();
            emit_newline(2);
            break;
        }
        case STMT_VAR_DECLARATION: {
            emit_instr("push rax");
            emit_instr("push rip");
            break;
        }
        case STMT_BLOCK: {
            AVEC_FOREACH(bstmt, tstmt->as.block) {
                emit_tstmt(bstmt);
            }
            break;
        }
        deafult: break;
    }
}

static void emit_texpr(texpr_t *texpr) {
    switch(texpr->kind) {
        case TEXPR_LITERAL: {
            switch(texpr->as.literal.kind) {
                case LIT_CHAR: emit_instr("mov rax, %hhu", texpr->as.literal._char); break;
                case LIT_BOOL: emit_instr("mov rax, %bool", texpr->as.literal._bool); break;
                case LIT_INTEGER: emit_instr("mov rax, %zu", texpr->as.literal._int); break;
                case LIT_FLOAT: emit_instr("mov rax, %f", texpr->as.literal._float); break;
                case LIT_STRING: break; // TODO, we have to implement slicing
                break;
            }
            break;
        }
        case TEXPR_UNARY: {
            break;
        }
        case TEXPR_BINARY: {
            switch(texpr->as.binary.op.kind) {
                case TOKEN_PLUS: {
                    emit_texpr(texpr->as.binary.left);
                    emit_instr("push rax");

                    emit_texpr(texpr->as.binary.right);

                    emit_instr("pop rbx");
                    emit_instr("add rax, rbx");
                }
                case TOKEN_MINUS: {
                    emit_texpr(texpr->as.binary.left);
                    emit_instr("push rax");

                    emit_texpr(texpr->as.binary.right);

                    emit_instr("pop rbx");
                    emit_instr("sub rax, rbx");
                }
                case TOKEN_STAR: {
                    emit_texpr(texpr->as.binary.left);
                    emit_instr("push rax");

                    emit_texpr(texpr->as.binary.right);

                    emit_instr("pop rbx");
                    emit_instr("mul rax, rbx");
                }
                case TOKEN_SLASH: {
                    emit_texpr(texpr->as.binary.left);
                    emit_instr("push rax");

                    emit_texpr(texpr->as.binary.right);

                    emit_instr("pop rbx");

                    if(texpr->type->kind == TY_UINT) {
                        emit_instr("udiv rax, rbx");
                    } else {
                        emit_instr("idiv rax, rbx");
                    }
                }
                default: {
                    fprintf(stderr, "cannot compile %s binary operator", 
                            token_kind_to_str(texpr->as.binary.op.kind));
                    exit(EINVAL);
                }
            }
            break;
        }
        default: break;
    }
}

static void scope_push() {
    asmscope_t *scope = malloc(sizeof *scope);
    *scope = (asmscope_t) {
        .locals = local_map_make(),
        .next_local_off = 0,
        .parent = emitter.scope
    };
    emitter.scope = scope;
}

static void scope_put(const char *identifier, size_t offset) {
    if(emitter.scope == nullptr) {
        fprintf(stderr, "cannot push to null emitter scope");
        exit(EINVAL);
    }

    local_map_put(emitter.scope->locals, identifier, offset);
}

static void scope_pop() {
    if(emitter.scope == nullptr) {
        fprintf(stderr, "cannot pop null emitter scope");
        exit(EINVAL);
    }
    local_map_destroy(emitter.scope->locals);
    free(emitter.scope);

    emitter.scope = emitter.scope->parent;
}

static void vemit(const char *fmt, const char *suffix, va_list args) {
    vfprintf(emitter.output, fmt, args);
    fputs(suffix, emitter.output);
}

static void emit_string(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(emitter.output, fmt, args);
    va_end(args);
    fputc('\n', emitter.output);
}

static void emit_label(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(emitter.output, fmt, args);
    va_end(args);
    fputs(":\n", emitter.output);
}

static void emit_instr(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs("    ", emitter.output);
    vfprintf(emitter.output, fmt, args);
    va_end(args);
    fputc('\n', emitter.output);
}

static void emit_newline(size_t c) {
    for(size_t i = 0; i < c; i++) {
        fprintf(emitter.output, "\n");
    }
}

static void get_identifier(char *dest, size_t max, span_t span) {
    snprintf(dest, max, "%.*s", (int)span.length, span.src);
}

