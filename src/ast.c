#include "jsrt/ast.h"

#include <stdlib.h>

#include "jsrt/memory.h"

void string_array_init(StringArray *array) {
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void string_array_push(StringArray *array, char *value) {
    if (array->count == array->capacity) {
        size_t next_capacity = array->capacity == 0 ? 8U : array->capacity * 2U;
        array->items = jsrt_realloc(array->items, next_capacity * sizeof(*array->items));
        array->capacity = next_capacity;
    }
    array->items[array->count++] = value;
}

void string_array_free(StringArray *array) {
    size_t i;
    for (i = 0; i < array->count; ++i) {
        free(array->items[i]);
    }
    free(array->items);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void expr_array_init(ExprArray *array) {
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void expr_array_push(ExprArray *array, Expr *expr) {
    if (array->count == array->capacity) {
        size_t next_capacity = array->capacity == 0 ? 8U : array->capacity * 2U;
        array->items = jsrt_realloc(array->items, next_capacity * sizeof(*array->items));
        array->capacity = next_capacity;
    }
    array->items[array->count++] = expr;
}

void expr_array_free(ExprArray *array) {
    size_t i;
    for (i = 0; i < array->count; ++i) {
        expr_destroy(array->items[i]);
    }
    free(array->items);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void object_property_array_init(ObjectPropertyArray *array) {
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void object_property_array_push(ObjectPropertyArray *array, ObjectProperty property) {
    if (array->count == array->capacity) {
        size_t next_capacity = array->capacity == 0 ? 8U : array->capacity * 2U;
        array->items = jsrt_realloc(array->items, next_capacity * sizeof(*array->items));
        array->capacity = next_capacity;
    }
    array->items[array->count++] = property;
}

void object_property_array_free(ObjectPropertyArray *array) {
    size_t i;
    for (i = 0; i < array->count; ++i) {
        free(array->items[i].key);
        expr_destroy(array->items[i].value);
    }
    free(array->items);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void switch_case_array_init(SwitchCaseArray *array) {
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void switch_case_array_push(SwitchCaseArray *array, SwitchCase switch_case) {
    if (array->count == array->capacity) {
        size_t next_capacity = array->capacity == 0 ? 8U : array->capacity * 2U;
        array->items = jsrt_realloc(array->items, next_capacity * sizeof(*array->items));
        array->capacity = next_capacity;
    }
    array->items[array->count++] = switch_case;
}

void switch_case_array_free(SwitchCaseArray *array) {
    size_t i;
    for (i = 0; i < array->count; ++i) {
        expr_destroy(array->items[i].value);
        stmt_array_free(&array->items[i].statements);
    }
    free(array->items);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void stmt_array_init(StmtArray *array) {
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void stmt_array_push(StmtArray *array, Stmt *stmt) {
    if (array->count == array->capacity) {
        size_t next_capacity = array->capacity == 0 ? 8U : array->capacity * 2U;
        array->items = jsrt_realloc(array->items, next_capacity * sizeof(*array->items));
        array->capacity = next_capacity;
    }
    array->items[array->count++] = stmt;
}

void stmt_array_free(StmtArray *array) {
    size_t i;
    for (i = 0; i < array->count; ++i) {
        stmt_destroy(array->items[i]);
    }
    free(array->items);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

static Expr *alloc_expr(ExprKind kind, int line, int column) {
    Expr *expr = jsrt_malloc(sizeof(*expr));
    expr->kind = kind;
    expr->line = line;
    expr->column = column;
    return expr;
}

static Stmt *alloc_stmt(StmtKind kind, int line, int column) {
    Stmt *stmt = jsrt_malloc(sizeof(*stmt));
    stmt->kind = kind;
    stmt->line = line;
    stmt->column = column;
    return stmt;
}

Expr *ast_new_literal_number(double value, int line, int column) {
    Expr *expr = alloc_expr(EXPR_LITERAL, line, column);
    expr->as.literal.kind = LITERAL_NUMBER;
    expr->as.literal.number = value;
    expr->as.literal.boolean = false;
    expr->as.literal.string = NULL;
    return expr;
}

Expr *ast_new_literal_bool(bool value, int line, int column) {
    Expr *expr = alloc_expr(EXPR_LITERAL, line, column);
    expr->as.literal.kind = LITERAL_BOOL;
    expr->as.literal.number = 0.0;
    expr->as.literal.boolean = value;
    expr->as.literal.string = NULL;
    return expr;
}

Expr *ast_new_literal_null(int line, int column) {
    Expr *expr = alloc_expr(EXPR_LITERAL, line, column);
    expr->as.literal.kind = LITERAL_NULL;
    expr->as.literal.number = 0.0;
    expr->as.literal.boolean = false;
    expr->as.literal.string = NULL;
    return expr;
}

Expr *ast_new_literal_string(const char *value, int line, int column) {
    Expr *expr = alloc_expr(EXPR_LITERAL, line, column);
    expr->as.literal.kind = LITERAL_STRING;
    expr->as.literal.number = 0.0;
    expr->as.literal.boolean = false;
    expr->as.literal.string = jsrt_strdup(value);
    return expr;
}

Expr *ast_new_identifier(const char *name, int line, int column) {
    Expr *expr = alloc_expr(EXPR_IDENTIFIER, line, column);
    expr->as.identifier.name = jsrt_strdup(name);
    return expr;
}

Expr *ast_new_unary(TokenType op, Expr *operand, int line, int column) {
    Expr *expr = alloc_expr(EXPR_UNARY, line, column);
    expr->as.unary.op = op;
    expr->as.unary.operand = operand;
    return expr;
}

Expr *ast_new_binary(Expr *left, TokenType op, Expr *right, int line, int column) {
    Expr *expr = alloc_expr(EXPR_BINARY, line, column);
    expr->as.binary.left = left;
    expr->as.binary.op = op;
    expr->as.binary.right = right;
    return expr;
}

Expr *ast_new_logical(Expr *left, TokenType op, Expr *right, int line, int column) {
    Expr *expr = alloc_expr(EXPR_LOGICAL, line, column);
    expr->as.logical.left = left;
    expr->as.logical.op = op;
    expr->as.logical.right = right;
    return expr;
}

Expr *ast_new_assign(Expr *target, Expr *value, int line, int column) {
    Expr *expr = alloc_expr(EXPR_ASSIGN, line, column);
    expr->as.assign.target = target;
    expr->as.assign.value = value;
    return expr;
}

Expr *ast_new_call(Expr *callee, ExprArray arguments, int line, int column) {
    Expr *expr = alloc_expr(EXPR_CALL, line, column);
    expr->as.call.callee = callee;
    expr->as.call.arguments = arguments;
    return expr;
}

Expr *ast_new_this(int line, int column) {
    return alloc_expr(EXPR_THIS, line, column);
}

Expr *ast_new_new(Expr *callee, ExprArray arguments, int line, int column) {
    Expr *expr = alloc_expr(EXPR_NEW, line, column);
    expr->as.new_expr.callee = callee;
    expr->as.new_expr.arguments = arguments;
    return expr;
}

Expr *ast_new_function_expr(const char *name, StringArray params, StmtArray body, int line, int column) {
    Expr *expr = alloc_expr(EXPR_FUNCTION, line, column);
    expr->as.function.name = name != NULL ? jsrt_strdup(name) : NULL;
    expr->as.function.params = params;
    expr->as.function.body = body;
    return expr;
}

Expr *ast_new_array_literal(ExprArray elements, int line, int column) {
    Expr *expr = alloc_expr(EXPR_ARRAY, line, column);
    expr->as.array.elements = elements;
    return expr;
}

Expr *ast_new_object_literal(ObjectPropertyArray properties, int line, int column) {
    Expr *expr = alloc_expr(EXPR_OBJECT, line, column);
    expr->as.object.properties = properties;
    return expr;
}

Expr *ast_new_index(Expr *target, Expr *index, int line, int column) {
    Expr *expr = alloc_expr(EXPR_INDEX, line, column);
    expr->as.index.target = target;
    expr->as.index.index = index;
    return expr;
}

Expr *ast_new_property(Expr *target, const char *name, int line, int column) {
    Expr *expr = alloc_expr(EXPR_PROPERTY, line, column);
    expr->as.property.target = target;
    expr->as.property.name = jsrt_strdup(name);
    return expr;
}

Stmt *ast_new_expr_stmt(Expr *expression, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_EXPR, line, column);
    stmt->as.expr.expression = expression;
    return stmt;
}

Stmt *ast_new_var_stmt(const char *name, Expr *initializer, bool is_const, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_VAR, line, column);
    stmt->as.var.name = jsrt_strdup(name);
    stmt->as.var.initializer = initializer;
    stmt->as.var.is_const = is_const;
    return stmt;
}

Stmt *ast_new_block_stmt(StmtArray statements, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_BLOCK, line, column);
    stmt->as.block.statements = statements;
    return stmt;
}

Stmt *ast_new_if_stmt(Expr *condition, Stmt *then_branch, Stmt *else_branch, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_IF, line, column);
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_branch = then_branch;
    stmt->as.if_stmt.else_branch = else_branch;
    return stmt;
}

Stmt *ast_new_do_while_stmt(Stmt *body, Expr *condition, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_DO_WHILE, line, column);
    stmt->as.do_while_stmt.body = body;
    stmt->as.do_while_stmt.condition = condition;
    return stmt;
}

Stmt *ast_new_for_stmt(Stmt *init, Expr *condition, Expr *increment, Stmt *body, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_FOR, line, column);
    stmt->as.for_stmt.init = init;
    stmt->as.for_stmt.condition = condition;
    stmt->as.for_stmt.increment = increment;
    stmt->as.for_stmt.body = body;
    return stmt;
}

Stmt *ast_new_while_stmt(Expr *condition, Stmt *body, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_WHILE, line, column);
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    return stmt;
}

Stmt *ast_new_switch_stmt(Expr *expression, SwitchCaseArray cases, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_SWITCH, line, column);
    stmt->as.switch_stmt.expression = expression;
    stmt->as.switch_stmt.cases = cases;
    return stmt;
}

Stmt *ast_new_function_stmt(const char *name, StringArray params, StmtArray body, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_FUNCTION, line, column);
    stmt->as.function.name = jsrt_strdup(name);
    stmt->as.function.params = params;
    stmt->as.function.body = body;
    return stmt;
}

Stmt *ast_new_return_stmt(Expr *value, int line, int column) {
    Stmt *stmt = alloc_stmt(STMT_RETURN, line, column);
    stmt->as.return_stmt.value = value;
    return stmt;
}

Stmt *ast_new_break_stmt(int line, int column) {
    return alloc_stmt(STMT_BREAK, line, column);
}

Stmt *ast_new_continue_stmt(int line, int column) {
    return alloc_stmt(STMT_CONTINUE, line, column);
}

void expr_destroy(Expr *expr) {
    if (expr == NULL) {
        return;
    }

    switch (expr->kind) {
        case EXPR_LITERAL:
            if (expr->as.literal.kind == LITERAL_STRING) {
                free(expr->as.literal.string);
            }
            break;
        case EXPR_IDENTIFIER:
            free(expr->as.identifier.name);
            break;
        case EXPR_UNARY:
            expr_destroy(expr->as.unary.operand);
            break;
        case EXPR_BINARY:
            expr_destroy(expr->as.binary.left);
            expr_destroy(expr->as.binary.right);
            break;
        case EXPR_LOGICAL:
            expr_destroy(expr->as.logical.left);
            expr_destroy(expr->as.logical.right);
            break;
        case EXPR_ASSIGN:
            expr_destroy(expr->as.assign.target);
            expr_destroy(expr->as.assign.value);
            break;
        case EXPR_CALL:
            expr_destroy(expr->as.call.callee);
            expr_array_free(&expr->as.call.arguments);
            break;
        case EXPR_THIS:
            break;
        case EXPR_NEW:
            expr_destroy(expr->as.new_expr.callee);
            expr_array_free(&expr->as.new_expr.arguments);
            break;
        case EXPR_FUNCTION:
            free(expr->as.function.name);
            string_array_free(&expr->as.function.params);
            stmt_array_free(&expr->as.function.body);
            break;
        case EXPR_ARRAY:
            expr_array_free(&expr->as.array.elements);
            break;
        case EXPR_OBJECT:
            object_property_array_free(&expr->as.object.properties);
            break;
        case EXPR_INDEX:
            expr_destroy(expr->as.index.target);
            expr_destroy(expr->as.index.index);
            break;
        case EXPR_PROPERTY:
            expr_destroy(expr->as.property.target);
            free(expr->as.property.name);
            break;
    }

    free(expr);
}

void stmt_destroy(Stmt *stmt) {
    if (stmt == NULL) {
        return;
    }

    switch (stmt->kind) {
        case STMT_EXPR:
            expr_destroy(stmt->as.expr.expression);
            break;
        case STMT_VAR:
            free(stmt->as.var.name);
            expr_destroy(stmt->as.var.initializer);
            break;
        case STMT_BLOCK:
            stmt_array_free(&stmt->as.block.statements);
            break;
        case STMT_IF:
            expr_destroy(stmt->as.if_stmt.condition);
            stmt_destroy(stmt->as.if_stmt.then_branch);
            stmt_destroy(stmt->as.if_stmt.else_branch);
            break;
        case STMT_DO_WHILE:
            stmt_destroy(stmt->as.do_while_stmt.body);
            expr_destroy(stmt->as.do_while_stmt.condition);
            break;
        case STMT_FOR:
            stmt_destroy(stmt->as.for_stmt.init);
            expr_destroy(stmt->as.for_stmt.condition);
            expr_destroy(stmt->as.for_stmt.increment);
            stmt_destroy(stmt->as.for_stmt.body);
            break;
        case STMT_WHILE:
            expr_destroy(stmt->as.while_stmt.condition);
            stmt_destroy(stmt->as.while_stmt.body);
            break;
        case STMT_SWITCH:
            expr_destroy(stmt->as.switch_stmt.expression);
            switch_case_array_free(&stmt->as.switch_stmt.cases);
            break;
        case STMT_FUNCTION:
            free(stmt->as.function.name);
            string_array_free(&stmt->as.function.params);
            stmt_array_free(&stmt->as.function.body);
            break;
        case STMT_RETURN:
            expr_destroy(stmt->as.return_stmt.value);
            break;
        case STMT_BREAK:
        case STMT_CONTINUE:
            break;
    }

    free(stmt);
}
