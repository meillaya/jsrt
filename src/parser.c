#include "jsrt/parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/diagnostics.h"
#include "jsrt/memory.h"

typedef struct {
    const TokenArray *tokens;
    const char *source;
    size_t current;
    char *error_message;
    int error_count;
    bool panic_mode;
} Parser;

static const Token *peek(const Parser *parser) {
    return &parser->tokens->items[parser->current];
}

static const Token *previous(const Parser *parser) {
    return &parser->tokens->items[parser->current - 1U];
}

static bool is_at_end(const Parser *parser) {
    return peek(parser)->type == TOKEN_EOF;
}

static const Token *advance(Parser *parser) {
    if (!is_at_end(parser)) {
        ++parser->current;
    }
    return previous(parser);
}

static bool check(const Parser *parser, TokenType type) {
    if (is_at_end(parser)) {
        return type == TOKEN_EOF;
    }
    return peek(parser)->type == type;
}

static bool match(Parser *parser, TokenType type) {
    if (!check(parser, type)) {
        return false;
    }
    advance(parser);
    return true;
}

static void append_error(Parser *parser, char *diagnostic) {
    if (parser->error_message == NULL) {
        parser->error_message = diagnostic;
    } else {
        char *joined = jsrt_format("%s\n\n%s", parser->error_message, diagnostic);
        free(parser->error_message);
        free(diagnostic);
        parser->error_message = joined;
    }
    parser->error_count += 1;
}

static void set_error(Parser *parser, const Token *token, const char *message) {
    if (!parser->panic_mode) {
        int width = token->lexeme[0] == '\0' ? 1 : (int)strlen(token->lexeme);
        append_error(
            parser,
            format_source_diagnostic(
                "Parser error",
                parser->source,
                token->line,
                token->column,
                width,
                message
            )
        );
        parser->panic_mode = true;
    }
}

static void synchronize(Parser *parser) {
    parser->panic_mode = false;
    while (!is_at_end(parser)) {
        if (peek(parser)->type == TOKEN_RIGHT_BRACE) {
            advance(parser);
            return;
        }
        if (parser->current > 0 && previous(parser)->type == TOKEN_SEMICOLON) {
            return;
        }
        switch (peek(parser)->type) {
            case TOKEN_LET:
            case TOKEN_FUNCTION:
            case TOKEN_IF:
            case TOKEN_DO:
            case TOKEN_FOR:
            case TOKEN_WHILE:
            case TOKEN_SWITCH:
            case TOKEN_BREAK:
            case TOKEN_CONTINUE:
            case TOKEN_RETURN:
            case TOKEN_CASE:
            case TOKEN_DEFAULT:
                return;
            default:
                break;
        }
        advance(parser);
    }
}

static bool consume(Parser *parser, TokenType type, const char *message) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    set_error(parser, peek(parser), message);
    return false;
}

static Expr *parse_expression(Parser *parser);
static Stmt *parse_declaration(Parser *parser);
static Stmt *parse_statement(Parser *parser);
static Expr *parse_assignment(Parser *parser);
static Stmt *parse_var_declaration(Parser *parser, bool is_const);

static Expr *parse_function_expression(Parser *parser) {
    const Token *keyword = previous(parser);
    const char *name = NULL;
    StringArray params;
    StmtArray body;

    string_array_init(&params);
    stmt_array_init(&body);

    if (match(parser, TOKEN_IDENTIFIER)) {
        name = previous(parser)->lexeme;
    }

    if (!consume(parser, TOKEN_LEFT_PAREN, "expected '(' after 'function'")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            if (!consume(parser, TOKEN_IDENTIFIER, "expected parameter name")) {
                string_array_free(&params);
                stmt_array_free(&body);
                return NULL;
            }
            string_array_push(&params, jsrt_strdup(previous(parser)->lexeme));
        } while (match(parser, TOKEN_COMMA));
    }

    if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after parameters")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }
    if (!consume(parser, TOKEN_LEFT_BRACE, "expected '{' before function body")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }

    while (!check(parser, TOKEN_RIGHT_BRACE) && !is_at_end(parser)) {
        Stmt *statement = parse_declaration(parser);
        if (statement != NULL) {
            stmt_array_push(&body, statement);
        } else {
            synchronize(parser);
        }
    }

    if (!consume(parser, TOKEN_RIGHT_BRACE, "expected '}' after function body")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }

    return ast_new_function_expr(name, params, body, keyword->line, keyword->column);
}

static Expr *parse_array_literal(Parser *parser, const Token *open) {
    ExprArray elements;
    expr_array_init(&elements);

    if (!check(parser, TOKEN_RIGHT_BRACKET)) {
        do {
            Expr *element = parse_expression(parser);
            if (element == NULL) {
                expr_array_free(&elements);
                return NULL;
            }
            expr_array_push(&elements, element);
        } while (match(parser, TOKEN_COMMA));
    }

    if (!consume(parser, TOKEN_RIGHT_BRACKET, "expected ']' after array literal")) {
        expr_array_free(&elements);
        return NULL;
    }
    return ast_new_array_literal(elements, open->line, open->column);
}

static Expr *parse_object_literal(Parser *parser, const Token *open) {
    ObjectPropertyArray properties;
    object_property_array_init(&properties);

    if (!check(parser, TOKEN_RIGHT_BRACE)) {
        do {
            char *key;
            Expr *value;
            if (match(parser, TOKEN_IDENTIFIER) || match(parser, TOKEN_STRING)) {
                key = jsrt_strdup(previous(parser)->lexeme);
            } else {
                set_error(parser, peek(parser), "expected object property name");
                object_property_array_free(&properties);
                return NULL;
            }
            if (!consume(parser, TOKEN_COLON, "expected ':' after object property name")) {
                free(key);
                object_property_array_free(&properties);
                return NULL;
            }
            value = parse_expression(parser);
            if (value == NULL) {
                free(key);
                object_property_array_free(&properties);
                return NULL;
            }
            {
                ObjectProperty property;
                property.key = key;
                property.value = value;
                object_property_array_push(&properties, property);
            }
        } while (match(parser, TOKEN_COMMA));
    }

    if (!consume(parser, TOKEN_RIGHT_BRACE, "expected '}' after object literal")) {
        object_property_array_free(&properties);
        return NULL;
    }
    return ast_new_object_literal(properties, open->line, open->column);
}

static Expr *parse_primary(Parser *parser) {
    const Token *token;
    if (match(parser, TOKEN_NUMBER)) {
        token = previous(parser);
        return ast_new_literal_number(token->number, token->line, token->column);
    }
    if (match(parser, TOKEN_STRING)) {
        token = previous(parser);
        return ast_new_literal_string(token->lexeme, token->line, token->column);
    }
    if (match(parser, TOKEN_TRUE)) {
        token = previous(parser);
        return ast_new_literal_bool(true, token->line, token->column);
    }
    if (match(parser, TOKEN_FALSE)) {
        token = previous(parser);
        return ast_new_literal_bool(false, token->line, token->column);
    }
    if (match(parser, TOKEN_NULL)) {
        token = previous(parser);
        return ast_new_literal_null(token->line, token->column);
    }
    if (match(parser, TOKEN_IDENTIFIER)) {
        token = previous(parser);
        return ast_new_identifier(token->lexeme, token->line, token->column);
    }
    if (match(parser, TOKEN_FUNCTION)) {
        return parse_function_expression(parser);
    }
    if (match(parser, TOKEN_LEFT_PAREN)) {
        const Token *open = previous(parser);
        Expr *expr = parse_expression(parser);
        if (expr == NULL) {
            return NULL;
        }
        if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after expression")) {
            expr_destroy(expr);
            return NULL;
        }
        expr->line = open->line;
        expr->column = open->column;
        return expr;
    }
    if (match(parser, TOKEN_LEFT_BRACKET)) {
        return parse_array_literal(parser, previous(parser));
    }
    if (match(parser, TOKEN_LEFT_BRACE)) {
        return parse_object_literal(parser, previous(parser));
    }

    set_error(parser, peek(parser), "expected expression");
    return NULL;
}

static Expr *parse_postfix(Parser *parser) {
    Expr *expr = parse_primary(parser);
    if (expr == NULL) {
        return NULL;
    }

    for (;;) {
        if (match(parser, TOKEN_LEFT_PAREN)) {
            const Token *open = previous(parser);
            ExprArray arguments;
            expr_array_init(&arguments);
            if (!check(parser, TOKEN_RIGHT_PAREN)) {
                do {
                    Expr *argument = parse_expression(parser);
                    if (argument == NULL) {
                        expr_array_free(&arguments);
                        expr_destroy(expr);
                        return NULL;
                    }
                    expr_array_push(&arguments, argument);
                } while (match(parser, TOKEN_COMMA));
            }
            if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after arguments")) {
                expr_array_free(&arguments);
                expr_destroy(expr);
                return NULL;
            }
            expr = ast_new_call(expr, arguments, open->line, open->column);
            continue;
        }
        if (match(parser, TOKEN_LEFT_BRACKET)) {
            const Token *open = previous(parser);
            Expr *index = parse_expression(parser);
            if (index == NULL) {
                expr_destroy(expr);
                return NULL;
            }
            if (!consume(parser, TOKEN_RIGHT_BRACKET, "expected ']' after index expression")) {
                expr_destroy(expr);
                expr_destroy(index);
                return NULL;
            }
            expr = ast_new_index(expr, index, open->line, open->column);
            continue;
        }
        if (match(parser, TOKEN_DOT)) {
            const Token *dot = previous(parser);
            if (!consume(parser, TOKEN_IDENTIFIER, "expected property name after '.'")) {
                expr_destroy(expr);
                return NULL;
            }
            expr = ast_new_property(expr, previous(parser)->lexeme, dot->line, dot->column);
            continue;
        }
        break;
    }

    return expr;
}

static Expr *parse_unary(Parser *parser) {
    if (match(parser, TOKEN_BANG) || match(parser, TOKEN_MINUS)) {
        const Token *op = previous(parser);
        Expr *right = parse_unary(parser);
        if (right == NULL) {
            return NULL;
        }
        return ast_new_unary(op->type, right, op->line, op->column);
    }
    return parse_postfix(parser);
}

static Expr *parse_factor(Parser *parser) {
    Expr *expr = parse_unary(parser);
    while (expr != NULL && (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH))) {
        const Token *op = previous(parser);
        Expr *right = parse_unary(parser);
        if (right == NULL) {
            expr_destroy(expr);
            return NULL;
        }
        expr = ast_new_binary(expr, op->type, right, op->line, op->column);
    }
    return expr;
}

static Expr *parse_term(Parser *parser) {
    Expr *expr = parse_factor(parser);
    while (expr != NULL && (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS))) {
        const Token *op = previous(parser);
        Expr *right = parse_factor(parser);
        if (right == NULL) {
            expr_destroy(expr);
            return NULL;
        }
        expr = ast_new_binary(expr, op->type, right, op->line, op->column);
    }
    return expr;
}

static Expr *parse_comparison(Parser *parser) {
    Expr *expr = parse_term(parser);
    while (expr != NULL && (match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL) ||
            match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL))) {
        const Token *op = previous(parser);
        Expr *right = parse_term(parser);
        if (right == NULL) {
            expr_destroy(expr);
            return NULL;
        }
        expr = ast_new_binary(expr, op->type, right, op->line, op->column);
    }
    return expr;
}

static Expr *parse_equality(Parser *parser) {
    Expr *expr = parse_comparison(parser);
    while (expr != NULL && (match(parser, TOKEN_EQUAL_EQUAL) || match(parser, TOKEN_BANG_EQUAL))) {
        const Token *op = previous(parser);
        Expr *right = parse_comparison(parser);
        if (right == NULL) {
            expr_destroy(expr);
            return NULL;
        }
        expr = ast_new_binary(expr, op->type, right, op->line, op->column);
    }
    return expr;
}

static Expr *parse_logical_and(Parser *parser) {
    Expr *expr = parse_equality(parser);
    while (expr != NULL && match(parser, TOKEN_AND_AND)) {
        const Token *op = previous(parser);
        Expr *right = parse_equality(parser);
        if (right == NULL) {
            expr_destroy(expr);
            return NULL;
        }
        expr = ast_new_logical(expr, op->type, right, op->line, op->column);
    }
    return expr;
}

static Expr *parse_logical_or(Parser *parser) {
    Expr *expr = parse_logical_and(parser);
    while (expr != NULL && match(parser, TOKEN_OR_OR)) {
        const Token *op = previous(parser);
        Expr *right = parse_logical_and(parser);
        if (right == NULL) {
            expr_destroy(expr);
            return NULL;
        }
        expr = ast_new_logical(expr, op->type, right, op->line, op->column);
    }
    return expr;
}

static Expr *parse_assignment(Parser *parser) {
    Expr *expr = parse_logical_or(parser);
    if (expr == NULL) {
        return NULL;
    }

    if (match(parser, TOKEN_EQUAL)) {
        const Token *equals = previous(parser);
        Expr *value = parse_assignment(parser);
        if (value == NULL) {
            expr_destroy(expr);
            return NULL;
        }
        if (expr->kind != EXPR_IDENTIFIER && expr->kind != EXPR_PROPERTY && expr->kind != EXPR_INDEX) {
            expr_destroy(expr);
            expr_destroy(value);
            set_error(parser, equals, "invalid assignment target");
            return NULL;
        }
        {
            return ast_new_assign(expr, value, equals->line, equals->column);
        }
    }

    return expr;
}

static Expr *parse_expression(Parser *parser) {
    return parse_assignment(parser);
}

static Stmt *parse_block_statement(Parser *parser, int line, int column) {
    StmtArray statements;
    stmt_array_init(&statements);

    while (!check(parser, TOKEN_RIGHT_BRACE) && !is_at_end(parser)) {
        Stmt *statement = parse_declaration(parser);
        if (statement != NULL) {
            stmt_array_push(&statements, statement);
        } else {
            synchronize(parser);
        }
    }

    if (!consume(parser, TOKEN_RIGHT_BRACE, "expected '}' after block")) {
        stmt_array_free(&statements);
        return NULL;
    }

    return ast_new_block_stmt(statements, line, column);
}

static Stmt *parse_return_statement(Parser *parser) {
    const Token *keyword = previous(parser);
    Expr *value = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        value = parse_expression(parser);
        if (value == NULL) {
            return NULL;
        }
    }
    if (!consume(parser, TOKEN_SEMICOLON, "expected ';' after return value")) {
        expr_destroy(value);
        return NULL;
    }
    return ast_new_return_stmt(value, keyword->line, keyword->column);
}

static Stmt *parse_if_statement(Parser *parser) {
    const Token *keyword = previous(parser);
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch = NULL;

    if (!consume(parser, TOKEN_LEFT_PAREN, "expected '(' after 'if'")) {
        return NULL;
    }
    condition = parse_expression(parser);
    if (condition == NULL) {
        return NULL;
    }
    if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after condition")) {
        expr_destroy(condition);
        return NULL;
    }
    then_branch = parse_statement(parser);
    if (then_branch == NULL) {
        expr_destroy(condition);
        return NULL;
    }
    if (match(parser, TOKEN_ELSE)) {
        else_branch = parse_statement(parser);
        if (else_branch == NULL) {
            expr_destroy(condition);
            stmt_destroy(then_branch);
            return NULL;
        }
    }
    return ast_new_if_stmt(condition, then_branch, else_branch, keyword->line, keyword->column);
}

static Stmt *parse_do_while_statement(Parser *parser) {
    const Token *keyword = previous(parser);
    Stmt *body;
    Expr *condition;

    body = parse_statement(parser);
    if (body == NULL) {
        return NULL;
    }
    if (!consume(parser, TOKEN_WHILE, "expected 'while' after do-while body")) {
        stmt_destroy(body);
        return NULL;
    }
    if (!consume(parser, TOKEN_LEFT_PAREN, "expected '(' after 'while'")) {
        stmt_destroy(body);
        return NULL;
    }
    condition = parse_expression(parser);
    if (condition == NULL) {
        stmt_destroy(body);
        return NULL;
    }
    if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after condition")) {
        stmt_destroy(body);
        expr_destroy(condition);
        return NULL;
    }
    if (!consume(parser, TOKEN_SEMICOLON, "expected ';' after do-while condition")) {
        stmt_destroy(body);
        expr_destroy(condition);
        return NULL;
    }
    return ast_new_do_while_stmt(body, condition, keyword->line, keyword->column);
}

static Stmt *parse_switch_statement(Parser *parser) {
    const Token *keyword = previous(parser);
    Expr *expression;
    SwitchCaseArray cases;
    bool saw_default = false;

    switch_case_array_init(&cases);
    if (!consume(parser, TOKEN_LEFT_PAREN, "expected '(' after 'switch'")) {
        return NULL;
    }
    expression = parse_expression(parser);
    if (expression == NULL) {
        return NULL;
    }
    if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after switch expression")) {
        expr_destroy(expression);
        return NULL;
    }
    if (!consume(parser, TOKEN_LEFT_BRACE, "expected '{' before switch body")) {
        expr_destroy(expression);
        return NULL;
    }

    while (!check(parser, TOKEN_RIGHT_BRACE) && !is_at_end(parser)) {
        SwitchCase switch_case;
        stmt_array_init(&switch_case.statements);
        switch_case.value = NULL;
        switch_case.is_default = false;

        if (match(parser, TOKEN_CASE)) {
            switch_case.value = parse_expression(parser);
            if (switch_case.value == NULL) {
                switch_case_array_free(&cases);
                expr_destroy(expression);
                return NULL;
            }
            if (!consume(parser, TOKEN_COLON, "expected ':' after case expression")) {
                expr_destroy(switch_case.value);
                stmt_array_free(&switch_case.statements);
                switch_case_array_free(&cases);
                expr_destroy(expression);
                return NULL;
            }
        } else if (match(parser, TOKEN_DEFAULT)) {
            if (saw_default) {
                set_error(parser, previous(parser), "duplicate 'default' clause in switch");
                stmt_array_free(&switch_case.statements);
                switch_case_array_free(&cases);
                expr_destroy(expression);
                return NULL;
            }
            saw_default = true;
            switch_case.is_default = true;
            if (!consume(parser, TOKEN_COLON, "expected ':' after 'default'")) {
                stmt_array_free(&switch_case.statements);
                switch_case_array_free(&cases);
                expr_destroy(expression);
                return NULL;
            }
        } else {
            set_error(parser, peek(parser), "expected 'case', 'default', or '}' in switch");
            stmt_array_free(&switch_case.statements);
            switch_case_array_free(&cases);
            expr_destroy(expression);
            return NULL;
        }

        while (!check(parser, TOKEN_CASE) && !check(parser, TOKEN_DEFAULT) &&
               !check(parser, TOKEN_RIGHT_BRACE) && !is_at_end(parser)) {
            Stmt *statement = parse_declaration(parser);
            if (statement != NULL) {
                stmt_array_push(&switch_case.statements, statement);
            } else {
                synchronize(parser);
            }
        }

        switch_case_array_push(&cases, switch_case);
    }

    if (!consume(parser, TOKEN_RIGHT_BRACE, "expected '}' after switch body")) {
        switch_case_array_free(&cases);
        expr_destroy(expression);
        return NULL;
    }
    return ast_new_switch_stmt(expression, cases, keyword->line, keyword->column);
}

static Stmt *parse_for_statement(Parser *parser) {
    const Token *keyword = previous(parser);
    Stmt *init = NULL;
    Expr *condition = NULL;
    Expr *increment = NULL;
    Stmt *body;

    if (!consume(parser, TOKEN_LEFT_PAREN, "expected '(' after 'for'")) {
        return NULL;
    }

    if (match(parser, TOKEN_SEMICOLON)) {
        init = NULL;
    } else if (match(parser, TOKEN_LET)) {
        init = parse_var_declaration(parser, false);
        if (init == NULL) {
            return NULL;
        }
    } else if (match(parser, TOKEN_CONST)) {
        init = parse_var_declaration(parser, true);
        if (init == NULL) {
            return NULL;
        }
    } else {
        Expr *init_expr = parse_expression(parser);
        if (init_expr == NULL) {
            return NULL;
        }
        if (!consume(parser, TOKEN_SEMICOLON, "expected ';' after for-loop initializer")) {
            expr_destroy(init_expr);
            return NULL;
        }
        init = ast_new_expr_stmt(init_expr, init_expr->line, init_expr->column);
    }

    if (!check(parser, TOKEN_SEMICOLON)) {
        condition = parse_expression(parser);
        if (condition == NULL) {
            stmt_destroy(init);
            return NULL;
        }
    }
    if (!consume(parser, TOKEN_SEMICOLON, "expected ';' after for-loop condition")) {
        stmt_destroy(init);
        expr_destroy(condition);
        return NULL;
    }

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        increment = parse_expression(parser);
        if (increment == NULL) {
            stmt_destroy(init);
            expr_destroy(condition);
            return NULL;
        }
    }
    if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after for-loop clauses")) {
        stmt_destroy(init);
        expr_destroy(condition);
        expr_destroy(increment);
        return NULL;
    }

    body = parse_statement(parser);
    if (body == NULL) {
        stmt_destroy(init);
        expr_destroy(condition);
        expr_destroy(increment);
        return NULL;
    }

    return ast_new_for_stmt(init, condition, increment, body, keyword->line, keyword->column);
}

static Stmt *parse_while_statement(Parser *parser) {
    const Token *keyword = previous(parser);
    Expr *condition;
    Stmt *body;

    if (!consume(parser, TOKEN_LEFT_PAREN, "expected '(' after 'while'")) {
        return NULL;
    }
    condition = parse_expression(parser);
    if (condition == NULL) {
        return NULL;
    }
    if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after condition")) {
        expr_destroy(condition);
        return NULL;
    }
    body = parse_statement(parser);
    if (body == NULL) {
        expr_destroy(condition);
        return NULL;
    }
    return ast_new_while_stmt(condition, body, keyword->line, keyword->column);
}

static Stmt *parse_break_statement(Parser *parser) {
    const Token *keyword = previous(parser);

    if (!consume(parser, TOKEN_SEMICOLON, "expected ';' after 'break'")) {
        return NULL;
    }

    return ast_new_break_stmt(keyword->line, keyword->column);
}

static Stmt *parse_continue_statement(Parser *parser) {
    const Token *keyword = previous(parser);

    if (!consume(parser, TOKEN_SEMICOLON, "expected ';' after 'continue'")) {
        return NULL;
    }

    return ast_new_continue_stmt(keyword->line, keyword->column);
}

static Stmt *parse_expression_statement(Parser *parser) {
    Expr *expression = parse_expression(parser);
    if (expression == NULL) {
        return NULL;
    }
    if (!consume(parser, TOKEN_SEMICOLON, "expected ';' after expression")) {
        expr_destroy(expression);
        return NULL;
    }
    return ast_new_expr_stmt(expression, expression->line, expression->column);
}

static Stmt *parse_statement(Parser *parser) {
    if (match(parser, TOKEN_IF)) {
        return parse_if_statement(parser);
    }
    if (match(parser, TOKEN_DO)) {
        return parse_do_while_statement(parser);
    }
    if (match(parser, TOKEN_SWITCH)) {
        return parse_switch_statement(parser);
    }
    if (match(parser, TOKEN_FOR)) {
        return parse_for_statement(parser);
    }
    if (match(parser, TOKEN_WHILE)) {
        return parse_while_statement(parser);
    }
    if (match(parser, TOKEN_BREAK)) {
        return parse_break_statement(parser);
    }
    if (match(parser, TOKEN_CONTINUE)) {
        return parse_continue_statement(parser);
    }
    if (match(parser, TOKEN_RETURN)) {
        return parse_return_statement(parser);
    }
    if (match(parser, TOKEN_LEFT_BRACE)) {
        const Token *open = previous(parser);
        return parse_block_statement(parser, open->line, open->column);
    }
    return parse_expression_statement(parser);
}

static Stmt *parse_var_declaration(Parser *parser, bool is_const) {
    const Token *keyword = previous(parser);
    const Token *name_token;
    Expr *initializer = NULL;
    if (!consume(parser, TOKEN_IDENTIFIER, "expected variable name")) {
        return NULL;
    }
    name_token = previous(parser);
    if (match(parser, TOKEN_EQUAL)) {
        initializer = parse_expression(parser);
        if (initializer == NULL) {
            return NULL;
        }
    } else if (is_const) {
        set_error(parser, peek(parser), "const declarations require an initializer");
        return NULL;
    }
    if (!consume(parser, TOKEN_SEMICOLON, "expected ';' after variable declaration")) {
        expr_destroy(initializer);
        return NULL;
    }
    return ast_new_var_stmt(name_token->lexeme, initializer, is_const, keyword->line, keyword->column);
}

static Stmt *parse_function_declaration(Parser *parser) {
    const Token *keyword = previous(parser);
    const Token *name_token;
    StringArray params;
    StmtArray body;

    string_array_init(&params);
    stmt_array_init(&body);

    if (!consume(parser, TOKEN_IDENTIFIER, "expected function name")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }
    name_token = previous(parser);

    if (!consume(parser, TOKEN_LEFT_PAREN, "expected '(' after function name")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            if (!consume(parser, TOKEN_IDENTIFIER, "expected parameter name")) {
                string_array_free(&params);
                stmt_array_free(&body);
                return NULL;
            }
            string_array_push(&params, jsrt_strdup(previous(parser)->lexeme));
        } while (match(parser, TOKEN_COMMA));
    }

    if (!consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after parameters")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }
    if (!consume(parser, TOKEN_LEFT_BRACE, "expected '{' before function body")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }

    while (!check(parser, TOKEN_RIGHT_BRACE) && !is_at_end(parser)) {
        Stmt *statement = parse_declaration(parser);
        if (statement != NULL) {
            stmt_array_push(&body, statement);
        } else {
            synchronize(parser);
        }
    }

    if (!consume(parser, TOKEN_RIGHT_BRACE, "expected '}' after function body")) {
        string_array_free(&params);
        stmt_array_free(&body);
        return NULL;
    }

    return ast_new_function_stmt(name_token->lexeme, params, body, keyword->line, keyword->column);
}

static Stmt *parse_declaration(Parser *parser) {
    if (match(parser, TOKEN_FUNCTION)) {
        return parse_function_declaration(parser);
    }
    if (match(parser, TOKEN_LET)) {
        return parse_var_declaration(parser, false);
    }
    if (match(parser, TOKEN_CONST)) {
        return parse_var_declaration(parser, true);
    }
    return parse_statement(parser);
}

int parse_tokens(const TokenArray *tokens, const char *source, StmtArray *program, char **error_message) {
    Parser parser;
    parser.tokens = tokens;
    parser.source = source;
    parser.current = 0;
    parser.error_message = NULL;
    parser.error_count = 0;
    parser.panic_mode = false;

    stmt_array_init(program);

    while (!is_at_end(&parser)) {
        Stmt *statement = parse_declaration(&parser);
        if (statement != NULL) {
            stmt_array_push(program, statement);
        } else {
            synchronize(&parser);
        }
    }

    if (parser.error_count > 0) {
        stmt_array_free(program);
        *error_message = parser.error_message;
        return 0;
    }

    *error_message = NULL;
    return 1;
}
