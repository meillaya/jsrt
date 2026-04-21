#include "jsrt/compiler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/diagnostics.h"
#include "jsrt/memory.h"

typedef struct {
    const char *name;
    int depth;
    size_t slot;
} Local;

typedef struct Compiler {
    BytecodeProgram *program;
    BytecodeFunction *function;
    const char *source;
    struct Compiler *parent;
    Local *locals;
    size_t local_count;
    size_t local_capacity;
    size_t next_slot;
    int scope_depth;
    bool is_script;
} Compiler;

static bool compile_statement(Compiler *compiler, const Stmt *stmt, char **error_message);
static bool compile_expression(Compiler *compiler, const Expr *expr, char **error_message);

static bool set_compile_error(const char *source, int line, int column, int width, const char *message, char **error_message) {
    if (error_message == NULL) {
        return false;
    }
    free(*error_message);
    *error_message = format_source_diagnostic("Compiler error", source, line, column, width, message);
    return false;
}

static bool set_compile_error_take(const char *source, int line, int column, int width, char *message, char **error_message) {
    bool ok = set_compile_error(source, line, column, width, message, error_message);
    free(message);
    return ok;
}

static BytecodeFunction *add_function(BytecodeProgram *program, const char *name, size_t arity) {
    BytecodeFunction *function;
    if (program->function_count == program->function_capacity) {
        size_t new_capacity = program->function_capacity == 0 ? 4U : program->function_capacity * 2U;
        program->functions = jsrt_realloc(program->functions, sizeof(*program->functions) * new_capacity);
        program->function_capacity = new_capacity;
    }
    function = &program->functions[program->function_count++];
    function->name = name != NULL ? jsrt_strdup(name) : NULL;
    function->arity = arity;
    function->local_count = 0;
    function->instructions = NULL;
    function->instruction_count = 0;
    function->instruction_capacity = 0;
    function->constants = NULL;
    function->constant_count = 0;
    function->constant_capacity = 0;
    return function;
}

static int add_global_symbol(BytecodeProgram *program, const char *name) {
    size_t i;
    for (i = 0; i < program->global_count; ++i) {
        if (strcmp(program->globals[i], name) == 0) {
            return (int)i;
        }
    }
    if (program->global_count == program->global_capacity) {
        size_t new_capacity = program->global_capacity == 0 ? 8U : program->global_capacity * 2U;
        program->globals = jsrt_realloc(program->globals, sizeof(*program->globals) * new_capacity);
        program->global_capacity = new_capacity;
    }
    program->globals[program->global_count] = jsrt_strdup(name);
    return (int)program->global_count++;
}

static int add_constant(BytecodeFunction *function, Value value) {
    if (function->constant_count == function->constant_capacity) {
        size_t new_capacity = function->constant_capacity == 0 ? 8U : function->constant_capacity * 2U;
        function->constants = jsrt_realloc(function->constants, sizeof(*function->constants) * new_capacity);
        function->constant_capacity = new_capacity;
    }
    function->constants[function->constant_count] = value;
    return (int)function->constant_count++;
}

static int emit_instruction(BytecodeFunction *function, BytecodeOp op, int operand) {
    if (function->instruction_count == function->instruction_capacity) {
        size_t new_capacity = function->instruction_capacity == 0 ? 16U : function->instruction_capacity * 2U;
        function->instructions = jsrt_realloc(function->instructions, sizeof(*function->instructions) * new_capacity);
        function->instruction_capacity = new_capacity;
    }
    function->instructions[function->instruction_count].op = op;
    function->instructions[function->instruction_count].operand = operand;
    return (int)function->instruction_count++;
}

static void patch_instruction(BytecodeFunction *function, int index, int operand) {
    function->instructions[(size_t)index].operand = operand;
}

static void compiler_begin_scope(Compiler *compiler) {
    compiler->scope_depth += 1;
}

static void compiler_end_scope(Compiler *compiler) {
    while (compiler->local_count > 0 && compiler->locals[compiler->local_count - 1U].depth == compiler->scope_depth) {
        compiler->local_count -= 1U;
    }
    compiler->scope_depth -= 1;
}

static int resolve_local(const Compiler *compiler, const char *name) {
    size_t i = compiler->local_count;
    while (i > 0) {
        i -= 1U;
        if (strcmp(compiler->locals[i].name, name) == 0) {
            return (int)compiler->locals[i].slot;
        }
    }
    return -1;
}

static bool resolve_captured_name(const Compiler *compiler, const char *name) {
    const Compiler *current = compiler->parent;
    while (current != NULL) {
        if (resolve_local(current, name) >= 0) {
            return true;
        }
        current = current->parent;
    }
    return false;
}

static bool declare_local(Compiler *compiler, const char *name, char **error_message) {
    size_t i = compiler->local_count;
    while (i > 0) {
        const Local *local;
        i -= 1U;
        local = &compiler->locals[i];
        if (local->depth != compiler->scope_depth) {
            break;
        }
        if (strcmp(local->name, name) == 0) {
            return set_compile_error(
                compiler->source,
                1,
                1,
                1,
                "duplicate local in compiler scope",
                error_message
            );
        }
    }
    if (compiler->local_count == compiler->local_capacity) {
        size_t new_capacity = compiler->local_capacity == 0 ? 8U : compiler->local_capacity * 2U;
        compiler->locals = jsrt_realloc(compiler->locals, sizeof(*compiler->locals) * new_capacity);
        compiler->local_capacity = new_capacity;
    }
    compiler->locals[compiler->local_count].name = name;
    compiler->locals[compiler->local_count].depth = compiler->scope_depth;
    compiler->locals[compiler->local_count].slot = compiler->next_slot;
    compiler->local_count += 1U;
    compiler->next_slot += 1U;
    if (compiler->next_slot > compiler->function->local_count) {
        compiler->function->local_count = compiler->next_slot;
    }
    return true;
}

static bool compile_identifier_load(Compiler *compiler, const char *name, int line, int column, char **error_message) {
    int slot = resolve_local(compiler, name);
    if (slot >= 0) {
        emit_instruction(compiler->function, BC_LOAD_LOCAL, slot);
        return true;
    }
    if (resolve_captured_name(compiler, name)) {
        return set_compile_error_take(
            compiler->source,
            line,
            column,
            (int)strlen(name),
            jsrt_format("closure capture is not supported by the bytecode MVP ('%s')", name),
            error_message
        );
    }
    emit_instruction(compiler->function, BC_LOAD_GLOBAL, add_global_symbol(compiler->program, name));
    return true;
}

static bool compile_identifier_store(Compiler *compiler, const char *name, int line, int column, char **error_message) {
    int slot = resolve_local(compiler, name);
    if (slot >= 0) {
        emit_instruction(compiler->function, BC_STORE_LOCAL, slot);
        return true;
    }
    if (resolve_captured_name(compiler, name)) {
        return set_compile_error_take(
            compiler->source,
            line,
            column,
            (int)strlen(name),
            jsrt_format("closure capture is not supported by the bytecode MVP ('%s')", name),
            error_message
        );
    }
    emit_instruction(compiler->function, BC_STORE_GLOBAL, add_global_symbol(compiler->program, name));
    return true;
}

static bool compile_statement_list(Compiler *compiler, const StmtArray *statements, char **error_message) {
    size_t i;
    for (i = 0; i < statements->count; ++i) {
        if (!compile_statement(compiler, statements->items[i], error_message)) {
            return false;
        }
    }
    return true;
}

static bool compile_function_body(Compiler *parent, const char *name, const StringArray *params, const StmtArray *body, int line, int column, char **error_message, int *out_index) {
    Compiler child;
    BytecodeFunction *function = add_function(parent->program, name != NULL ? name : "<anonymous>", params->count);
    size_t i;

    child.program = parent->program;
    child.function = function;
    child.source = parent->source;
    child.parent = parent;
    child.locals = NULL;
    child.local_count = 0;
    child.local_capacity = 0;
    child.next_slot = 0;
    child.scope_depth = 1;
    child.is_script = false;

    for (i = 0; i < params->count; ++i) {
        if (!declare_local(&child, params->items[i], error_message)) {
            free(child.locals);
            return false;
        }
    }

    for (i = 0; i < body->count; ++i) {
        const Stmt *stmt = body->items[i];
        if (stmt->kind == STMT_FUNCTION) {
            free(child.locals);
            return set_compile_error(
                parent->source,
                stmt->line,
                stmt->column,
                1,
                "nested function declarations are deferred in the bytecode MVP",
                error_message
            );
        }
    }

    if (!compile_statement_list(&child, body, error_message)) {
        free(child.locals);
        return false;
    }
    emit_instruction(function, BC_NULL, 0);
    emit_instruction(function, BC_RETURN, 0);
    free(child.locals);
    *out_index = (int)(parent->program->function_count - 1U);
    (void)line;
    (void)column;
    return true;
}

static bool compile_logical(Compiler *compiler, const Expr *expr, char **error_message) {
    int jump_index;
    int end_index;

    if (!compile_expression(compiler, expr->as.logical.left, error_message)) {
        return false;
    }

    if (expr->as.logical.op == TOKEN_AND_AND) {
        jump_index = emit_instruction(compiler->function, BC_JUMP_IF_FALSE, -1);
        emit_instruction(compiler->function, BC_POP, 0);
        if (!compile_expression(compiler, expr->as.logical.right, error_message)) {
            return false;
        }
        patch_instruction(compiler->function, jump_index, (int)compiler->function->instruction_count);
        return true;
    }

    if (expr->as.logical.op == TOKEN_OR_OR) {
        jump_index = emit_instruction(compiler->function, BC_JUMP_IF_FALSE, -1);
        end_index = emit_instruction(compiler->function, BC_JUMP, -1);
        patch_instruction(compiler->function, jump_index, (int)compiler->function->instruction_count);
        emit_instruction(compiler->function, BC_POP, 0);
        if (!compile_expression(compiler, expr->as.logical.right, error_message)) {
            return false;
        }
        patch_instruction(compiler->function, end_index, (int)compiler->function->instruction_count);
        return true;
    }

    return set_compile_error(compiler->source, expr->line, expr->column, 2, "unsupported logical operator in compiler", error_message);
}

static bool compile_expression(Compiler *compiler, const Expr *expr, char **error_message) {
    switch (expr->kind) {
        case EXPR_LITERAL:
            switch (expr->as.literal.kind) {
                case LITERAL_NUMBER:
                    emit_instruction(compiler->function, BC_CONST, add_constant(compiler->function, value_number(expr->as.literal.number)));
                    return true;
                case LITERAL_BOOL:
                    emit_instruction(compiler->function, expr->as.literal.boolean ? BC_TRUE : BC_FALSE, 0);
                    return true;
                case LITERAL_NULL:
                    emit_instruction(compiler->function, BC_NULL, 0);
                    return true;
                case LITERAL_STRING:
                    emit_instruction(compiler->function, BC_CONST, add_constant(compiler->function, value_string(expr->as.literal.string)));
                    return true;
            }
            break;
        case EXPR_IDENTIFIER:
            return compile_identifier_load(compiler, expr->as.identifier.name, expr->line, expr->column, error_message);
        case EXPR_UNARY:
            if (!compile_expression(compiler, expr->as.unary.operand, error_message)) {
                return false;
            }
            if (expr->as.unary.op == TOKEN_MINUS) {
                emit_instruction(compiler->function, BC_NEG, 0);
                return true;
            }
            if (expr->as.unary.op == TOKEN_BANG) {
                emit_instruction(compiler->function, BC_NOT, 0);
                return true;
            }
            return set_compile_error(compiler->source, expr->line, expr->column, 1, "unsupported unary operator in compiler", error_message);
        case EXPR_BINARY:
            if (!compile_expression(compiler, expr->as.binary.left, error_message) ||
                !compile_expression(compiler, expr->as.binary.right, error_message)) {
                return false;
            }
            switch (expr->as.binary.op) {
                case TOKEN_PLUS: emit_instruction(compiler->function, BC_ADD, 0); return true;
                case TOKEN_MINUS: emit_instruction(compiler->function, BC_SUB, 0); return true;
                case TOKEN_STAR: emit_instruction(compiler->function, BC_MUL, 0); return true;
                case TOKEN_SLASH: emit_instruction(compiler->function, BC_DIV, 0); return true;
                case TOKEN_EQUAL_EQUAL: emit_instruction(compiler->function, BC_EQ, 0); return true;
                case TOKEN_BANG_EQUAL: emit_instruction(compiler->function, BC_NEQ, 0); return true;
                case TOKEN_LESS: emit_instruction(compiler->function, BC_LT, 0); return true;
                case TOKEN_LESS_EQUAL: emit_instruction(compiler->function, BC_LTE, 0); return true;
                case TOKEN_GREATER: emit_instruction(compiler->function, BC_GT, 0); return true;
                case TOKEN_GREATER_EQUAL: emit_instruction(compiler->function, BC_GTE, 0); return true;
                default:
                    return set_compile_error(compiler->source, expr->line, expr->column, 1, "unsupported binary operator in compiler", error_message);
            }
        case EXPR_LOGICAL:
            return compile_logical(compiler, expr, error_message);
        case EXPR_ASSIGN:
            if (expr->as.assign.target->kind != EXPR_IDENTIFIER) {
                return set_compile_error(compiler->source, expr->line, expr->column, 1, "aggregate assignment is deferred in the bytecode MVP", error_message);
            }
            if (!compile_expression(compiler, expr->as.assign.value, error_message)) {
                return false;
            }
            emit_instruction(compiler->function, BC_DUP, 0);
            return compile_identifier_store(compiler, expr->as.assign.target->as.identifier.name, expr->line, expr->column, error_message);
        case EXPR_CALL: {
            size_t i;
            if (!compile_expression(compiler, expr->as.call.callee, error_message)) {
                return false;
            }
            for (i = 0; i < expr->as.call.arguments.count; ++i) {
                if (!compile_expression(compiler, expr->as.call.arguments.items[i], error_message)) {
                    return false;
                }
            }
            emit_instruction(compiler->function, BC_CALL, (int)expr->as.call.arguments.count);
            return true;
        }
        case EXPR_FUNCTION:
            return set_compile_error(compiler->source, expr->line, expr->column, 1, "function expressions are deferred in the bytecode MVP", error_message);
        case EXPR_ARRAY:
        case EXPR_OBJECT:
        case EXPR_INDEX:
        case EXPR_PROPERTY:
            return set_compile_error(compiler->source, expr->line, expr->column, 1, "aggregate bytecode lowering is deferred in the bytecode MVP", error_message);
    }

    return set_compile_error(compiler->source, expr->line, expr->column, 1, "unsupported expression in compiler", error_message);
}

static bool compile_variable_declaration(Compiler *compiler, const Stmt *stmt, char **error_message) {
    if (stmt->as.var.initializer != NULL) {
        if (!compile_expression(compiler, stmt->as.var.initializer, error_message)) {
            return false;
        }
    } else {
        emit_instruction(compiler->function, BC_NULL, 0);
    }

    if (compiler->is_script && compiler->scope_depth == 0) {
        emit_instruction(compiler->function, BC_DEFINE_GLOBAL, add_global_symbol(compiler->program, stmt->as.var.name));
        return true;
    }

    if (!declare_local(compiler, stmt->as.var.name, error_message)) {
        return false;
    }
    emit_instruction(compiler->function, BC_STORE_LOCAL, (int)(compiler->locals[compiler->local_count - 1U].slot));
    return true;
}

static bool compile_if_statement(Compiler *compiler, const Stmt *stmt, char **error_message) {
    int false_jump;
    int end_jump = -1;
    if (!compile_expression(compiler, stmt->as.if_stmt.condition, error_message)) {
        return false;
    }
    false_jump = emit_instruction(compiler->function, BC_JUMP_IF_FALSE, -1);
    emit_instruction(compiler->function, BC_POP, 0);
    if (!compile_statement(compiler, stmt->as.if_stmt.then_branch, error_message)) {
        return false;
    }
    if (stmt->as.if_stmt.else_branch != NULL) {
        end_jump = emit_instruction(compiler->function, BC_JUMP, -1);
    }
    patch_instruction(compiler->function, false_jump, (int)compiler->function->instruction_count);
    emit_instruction(compiler->function, BC_POP, 0);
    if (stmt->as.if_stmt.else_branch != NULL) {
        if (!compile_statement(compiler, stmt->as.if_stmt.else_branch, error_message)) {
            return false;
        }
        patch_instruction(compiler->function, end_jump, (int)compiler->function->instruction_count);
    }
    return true;
}

static bool compile_while_statement(Compiler *compiler, const Stmt *stmt, char **error_message) {
    int loop_start = (int)compiler->function->instruction_count;
    int exit_jump;
    if (!compile_expression(compiler, stmt->as.while_stmt.condition, error_message)) {
        return false;
    }
    exit_jump = emit_instruction(compiler->function, BC_JUMP_IF_FALSE, -1);
    emit_instruction(compiler->function, BC_POP, 0);
    if (!compile_statement(compiler, stmt->as.while_stmt.body, error_message)) {
        return false;
    }
    emit_instruction(compiler->function, BC_LOOP, loop_start);
    patch_instruction(compiler->function, exit_jump, (int)compiler->function->instruction_count);
    emit_instruction(compiler->function, BC_POP, 0);
    return true;
}

static bool compile_for_statement(Compiler *compiler, const Stmt *stmt, char **error_message) {
    int loop_start;
    int exit_jump = -1;

    compiler_begin_scope(compiler);
    if (stmt->as.for_stmt.init != NULL) {
        if (!compile_statement(compiler, stmt->as.for_stmt.init, error_message)) {
            return false;
        }
    }

    loop_start = (int)compiler->function->instruction_count;
    if (stmt->as.for_stmt.condition != NULL) {
        if (!compile_expression(compiler, stmt->as.for_stmt.condition, error_message)) {
            return false;
        }
        exit_jump = emit_instruction(compiler->function, BC_JUMP_IF_FALSE, -1);
        emit_instruction(compiler->function, BC_POP, 0);
    }

    if (!compile_statement(compiler, stmt->as.for_stmt.body, error_message)) {
        return false;
    }

    if (stmt->as.for_stmt.increment != NULL) {
        if (!compile_expression(compiler, stmt->as.for_stmt.increment, error_message)) {
            return false;
        }
        emit_instruction(compiler->function, BC_POP, 0);
    }

    emit_instruction(compiler->function, BC_LOOP, loop_start);
    if (exit_jump >= 0) {
        patch_instruction(compiler->function, exit_jump, (int)compiler->function->instruction_count);
        emit_instruction(compiler->function, BC_POP, 0);
    }
    compiler_end_scope(compiler);
    return true;
}

static bool compile_statement(Compiler *compiler, const Stmt *stmt, char **error_message) {
    switch (stmt->kind) {
        case STMT_EXPR:
            if (!compile_expression(compiler, stmt->as.expr.expression, error_message)) {
                return false;
            }
            emit_instruction(compiler->function, BC_POP, 0);
            return true;
        case STMT_VAR:
            return compile_variable_declaration(compiler, stmt, error_message);
        case STMT_BLOCK:
            compiler_begin_scope(compiler);
            if (!compile_statement_list(compiler, &stmt->as.block.statements, error_message)) {
                return false;
            }
            compiler_end_scope(compiler);
            return true;
        case STMT_IF:
            return compile_if_statement(compiler, stmt, error_message);
        case STMT_WHILE:
            return compile_while_statement(compiler, stmt, error_message);
        case STMT_FOR:
            return compile_for_statement(compiler, stmt, error_message);
        case STMT_FUNCTION: {
            int function_index;
            if (!compile_function_body(
                    compiler,
                    stmt->as.function.name,
                    &stmt->as.function.params,
                    &stmt->as.function.body,
                    stmt->line,
                    stmt->column,
                    error_message,
                    &function_index)) {
                return false;
            }
            emit_instruction(compiler->function, BC_MAKE_FUNCTION, function_index);
            if (compiler->is_script && compiler->scope_depth == 0) {
                emit_instruction(compiler->function, BC_DEFINE_GLOBAL, add_global_symbol(compiler->program, stmt->as.function.name));
                return true;
            }
            if (!declare_local(compiler, stmt->as.function.name, error_message)) {
                return false;
            }
            emit_instruction(compiler->function, BC_STORE_LOCAL, (int)(compiler->locals[compiler->local_count - 1U].slot));
            return true;
        }
        case STMT_RETURN:
            if (stmt->as.return_stmt.value != NULL) {
                if (!compile_expression(compiler, stmt->as.return_stmt.value, error_message)) {
                    return false;
                }
            } else {
                emit_instruction(compiler->function, BC_NULL, 0);
            }
            emit_instruction(compiler->function, BC_RETURN, 0);
            return true;
        case STMT_DO_WHILE:
            return set_compile_error(compiler->source, stmt->line, stmt->column, 2, "do...while is deferred in the bytecode MVP", error_message);
        case STMT_SWITCH:
            return set_compile_error(compiler->source, stmt->line, stmt->column, 6, "switch is deferred in the bytecode MVP", error_message);
        case STMT_BREAK:
            return set_compile_error(compiler->source, stmt->line, stmt->column, 5, "break is deferred in the bytecode MVP", error_message);
        case STMT_CONTINUE:
            return set_compile_error(compiler->source, stmt->line, stmt->column, 8, "continue is deferred in the bytecode MVP", error_message);
    }

    return set_compile_error(compiler->source, stmt->line, stmt->column, 1, "unsupported statement in compiler", error_message);
}

bool compile_program(const StmtArray *program, const char *source, BytecodeProgram *out_program, char **error_message) {
    Compiler compiler;
    BytecodeFunction *script;

    if (error_message != NULL) {
        *error_message = NULL;
    }
    bytecode_program_init(out_program);
    script = add_function(out_program, "<script>", 0U);

    compiler.program = out_program;
    compiler.function = script;
    compiler.source = source;
    compiler.parent = NULL;
    compiler.locals = NULL;
    compiler.local_count = 0;
    compiler.local_capacity = 0;
    compiler.next_slot = 0;
    compiler.scope_depth = 0;
    compiler.is_script = true;

    if (!compile_statement_list(&compiler, program, error_message)) {
        free(compiler.locals);
        bytecode_program_free(out_program);
        return false;
    }

    emit_instruction(script, BC_NULL, 0);
    emit_instruction(script, BC_RETURN, 0);
    free(compiler.locals);
    return true;
}
