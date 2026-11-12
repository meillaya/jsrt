#include "jsrt/lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/diagnostics.h"
#include "jsrt/memory.h"

static void push_token(TokenArray *tokens, TokenType type, const char *start, size_t length, double number, int line, int column) {
    Token token;
    token.type = type;
    token.lexeme = jsrt_malloc(length + 1U);
    memcpy(token.lexeme, start, length);
    token.lexeme[length] = '\0';
    token.number = number;
    token.line = line;
    token.column = column;
    token_array_push(tokens, token);
}

static void push_owned_token(TokenArray *tokens, TokenType type, char *lexeme, double number, int line, int column) {
    Token token;
    token.type = type;
    token.lexeme = lexeme;
    token.number = number;
    token.line = line;
    token.column = column;
    token_array_push(tokens, token);
}

void token_array_init(TokenArray *array) {
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

void token_array_push(TokenArray *array, Token token) {
    if (array->count == array->capacity) {
        size_t next_capacity = array->capacity == 0 ? 16U : array->capacity * 2U;
        array->items = jsrt_realloc(array->items, next_capacity * sizeof(*array->items));
        array->capacity = next_capacity;
    }
    array->items[array->count++] = token;
}

void token_array_free(TokenArray *array) {
    size_t i;
    for (i = 0; i < array->count; ++i) {
        free(array->items[i].lexeme);
    }
    free(array->items);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_LEFT_PAREN: return "(";
        case TOKEN_RIGHT_PAREN: return ")";
        case TOKEN_LEFT_BRACE: return "{";
        case TOKEN_RIGHT_BRACE: return "}";
        case TOKEN_LEFT_BRACKET: return "[";
        case TOKEN_RIGHT_BRACKET: return "]";
        case TOKEN_COMMA: return ",";
        case TOKEN_DOT: return ".";
        case TOKEN_COLON: return ":";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_AND_AND: return "&&";
        case TOKEN_OR_OR: return "||";
        case TOKEN_BANG: return "!";
        case TOKEN_BANG_EQUAL: return "!=";
        case TOKEN_EQUAL: return "=";
        case TOKEN_EQUAL_EQUAL: return "==";
        case TOKEN_GREATER: return ">";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_LESS: return "<";
        case TOKEN_LESS_EQUAL: return "<=";
        case TOKEN_IDENTIFIER: return "identifier";
        case TOKEN_NUMBER: return "number";
        case TOKEN_STRING: return "string";
        case TOKEN_LET: return "let";
        case TOKEN_CONST: return "const";
        case TOKEN_FUNCTION: return "function";
        case TOKEN_CLASS: return "class";
        case TOKEN_EXTENDS: return "extends";
        case TOKEN_SUPER: return "super";
        case TOKEN_IMPORT: return "import";
        case TOKEN_EXPORT: return "export";
        case TOKEN_FROM: return "from";
        case TOKEN_AS: return "as";
        case TOKEN_RETURN: return "return";
        case TOKEN_IF: return "if";
        case TOKEN_ELSE: return "else";
        case TOKEN_DO: return "do";
        case TOKEN_FOR: return "for";
        case TOKEN_WHILE: return "while";
        case TOKEN_SWITCH: return "switch";
        case TOKEN_CASE: return "case";
        case TOKEN_DEFAULT: return "default";
        case TOKEN_BREAK: return "break";
        case TOKEN_CONTINUE: return "continue";
        case TOKEN_TRUE: return "true";
        case TOKEN_FALSE: return "false";
        case TOKEN_NULL: return "null";
        case TOKEN_THIS: return "this";
        case TOKEN_NEW: return "new";
        default: return "unknown";
    }
}

static TokenType identifier_type(const char *lexeme) {
    if (strcmp(lexeme, "let") == 0) {
        return TOKEN_LET;
    }
    if (strcmp(lexeme, "const") == 0) {
        return TOKEN_CONST;
    }
    if (strcmp(lexeme, "function") == 0) {
        return TOKEN_FUNCTION;
    }
    if (strcmp(lexeme, "class") == 0) {
        return TOKEN_CLASS;
    }
    if (strcmp(lexeme, "extends") == 0) {
        return TOKEN_EXTENDS;
    }
    if (strcmp(lexeme, "super") == 0) {
        return TOKEN_SUPER;
    }
    if (strcmp(lexeme, "import") == 0) {
        return TOKEN_IMPORT;
    }
    if (strcmp(lexeme, "export") == 0) {
        return TOKEN_EXPORT;
    }
    if (strcmp(lexeme, "from") == 0) {
        return TOKEN_FROM;
    }
    if (strcmp(lexeme, "as") == 0) {
        return TOKEN_AS;
    }
    if (strcmp(lexeme, "return") == 0) {
        return TOKEN_RETURN;
    }
    if (strcmp(lexeme, "if") == 0) {
        return TOKEN_IF;
    }
    if (strcmp(lexeme, "else") == 0) {
        return TOKEN_ELSE;
    }
    if (strcmp(lexeme, "do") == 0) {
        return TOKEN_DO;
    }
    if (strcmp(lexeme, "for") == 0) {
        return TOKEN_FOR;
    }
    if (strcmp(lexeme, "while") == 0) {
        return TOKEN_WHILE;
    }
    if (strcmp(lexeme, "switch") == 0) {
        return TOKEN_SWITCH;
    }
    if (strcmp(lexeme, "case") == 0) {
        return TOKEN_CASE;
    }
    if (strcmp(lexeme, "default") == 0) {
        return TOKEN_DEFAULT;
    }
    if (strcmp(lexeme, "break") == 0) {
        return TOKEN_BREAK;
    }
    if (strcmp(lexeme, "continue") == 0) {
        return TOKEN_CONTINUE;
    }
    if (strcmp(lexeme, "true") == 0) {
        return TOKEN_TRUE;
    }
    if (strcmp(lexeme, "false") == 0) {
        return TOKEN_FALSE;
    }
    if (strcmp(lexeme, "null") == 0) {
        return TOKEN_NULL;
    }
    if (strcmp(lexeme, "this") == 0) {
        return TOKEN_THIS;
    }
    if (strcmp(lexeme, "new") == 0) {
        return TOKEN_NEW;
    }
    return TOKEN_IDENTIFIER;
}

static int string_literal_error(
    const char *source,
    char **error_message,
    TokenArray *tokens,
    char *buffer,
    int token_line,
    int token_column,
    const char *message
) {
    free(buffer);
    *error_message = format_source_diagnostic("Lexer error", source, token_line, token_column, 1, message);
    token_array_free(tokens);
    return 0;
}

static bool hex_digit_value(char digit, unsigned int *out_value) {
    if (digit >= '0' && digit <= '9') {
        *out_value = (unsigned int)(digit - '0');
        return true;
    }
    if (digit >= 'a' && digit <= 'f') {
        *out_value = (unsigned int)(digit - 'a') + 10U;
        return true;
    }
    if (digit >= 'A' && digit <= 'F') {
        *out_value = (unsigned int)(digit - 'A') + 10U;
        return true;
    }
    return false;
}

static int lex_string_literal(
    const char *source,
    const char **cursor,
    int *column,
    TokenArray *tokens,
    char **error_message,
    int token_line,
    int token_column,
    char quote
) {
    size_t capacity = 16U;
    size_t length = 0U;
    char *buffer = jsrt_malloc(capacity);

    *cursor += 1;
    *column += 1;

    while (**cursor != '\0' && **cursor != quote) {
        char ch = **cursor;
        char appended[4];
        size_t appended_count = 1U;

        if (ch == '\n') {
            return string_literal_error(
                source,
                error_message,
                tokens,
                buffer,
                token_line,
                token_column,
                "unterminated string literal"
            );
        }
        if (ch == '\\') {
            *cursor += 1;
            *column += 1;
            ch = **cursor;
            if (ch == '\0' || ch == '\n') {
                return string_literal_error(
                    source,
                    error_message,
                    tokens,
                    buffer,
                    token_line,
                    token_column,
                    "unterminated string literal"
                );
            }
            switch (ch) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case 'r': ch = '\r'; break;
                case 'b': ch = '\b'; break;
                case 'f': ch = '\f'; break;
                case 'v': ch = '\v'; break;
                case '\'': ch = '\''; break;
                case '"': ch = '"'; break;
                case '\\': ch = '\\'; break;
                case 'x': {
                    unsigned int high;
                    unsigned int low;
                    unsigned int byte;
                    if (!hex_digit_value(cursor[0][1], &high) || !hex_digit_value(cursor[0][2], &low)) {
                        return string_literal_error(
                            source,
                            error_message,
                            tokens,
                            buffer,
                            token_line,
                            token_column,
                            "invalid hexadecimal escape sequence"
                        );
                    }
                    byte = (high << 4U) | low;
                    if (byte == 0U) {
                        return string_literal_error(
                            source,
                            error_message,
                            tokens,
                            buffer,
                            token_line,
                            token_column,
                            "NUL bytes are not supported in strings"
                        );
                    }
                    ch = (char)byte;
                    *cursor += 2;
                    *column += 2;
                    break;
                }
                case 'u': {
                    unsigned int codepoint = 0U;
                    unsigned int digit;
                    size_t index;
                    for (index = 1U; index <= 4U; ++index) {
                        if (!hex_digit_value(cursor[0][index], &digit)) {
                            return string_literal_error(
                                source,
                                error_message,
                                tokens,
                                buffer,
                                token_line,
                                token_column,
                                "invalid unicode escape sequence"
                            );
                        }
                        codepoint = (codepoint << 4U) | digit;
                    }
                    *cursor += 4;
                    *column += 4;
                    if (codepoint == 0U) {
                        return string_literal_error(
                            source,
                            error_message,
                            tokens,
                            buffer,
                            token_line,
                            token_column,
                            "NUL bytes are not supported in strings"
                        );
                    }
                    if (codepoint >= 0xD800U && codepoint <= 0xDFFFU) {
                        return string_literal_error(
                            source,
                            error_message,
                            tokens,
                            buffer,
                            token_line,
                            token_column,
                            "surrogate escape sequences are unsupported"
                        );
                    }
                    if (codepoint < 0x80U) {
                        ch = (char)codepoint;
                    } else if (codepoint < 0x800U) {
                        appended[0] = (char)(0xC0U | (codepoint >> 6U));
                        appended[1] = (char)(0x80U | (codepoint & 0x3FU));
                        appended_count = 2U;
                    } else {
                        appended[0] = (char)(0xE0U | (codepoint >> 12U));
                        appended[1] = (char)(0x80U | ((codepoint >> 6U) & 0x3FU));
                        appended[2] = (char)(0x80U | (codepoint & 0x3FU));
                        appended_count = 3U;
                    }
                    break;
                }
                default:
                    /* Identity escapes such as \q keep the character, which
                     * matches how JavaScript treats an unrecognized escape. */
                    break;
            }
        }
        if (length + 1U >= capacity) {
            capacity *= 2U;
            buffer = jsrt_realloc(buffer, capacity);
        }
        if (appended_count == 1U) {
            buffer[length++] = ch;
        } else {
            while (length + appended_count >= capacity) {
                capacity *= 2U;
                buffer = jsrt_realloc(buffer, capacity);
            }
            memcpy(buffer + length, appended, appended_count);
            length += appended_count;
        }
        *cursor += 1;
        *column += 1;
    }

    if (**cursor != quote) {
        return string_literal_error(
            source,
            error_message,
            tokens,
            buffer,
            token_line,
            token_column,
            "unterminated string literal"
        );
    }

    buffer[length] = '\0';
    *cursor += 1;
    *column += 1;
    push_owned_token(tokens, TOKEN_STRING, buffer, 0.0, token_line, token_column);
    return 1;
}

int lex_source(const char *source, TokenArray *tokens, char **error_message) {
    const char *cursor = source;
    int line = 1;
    int column = 1;

    *error_message = NULL;
    token_array_init(tokens);

    while (*cursor != '\0') {
        const char *token_start = cursor;
        int token_line = line;
        int token_column = column;

        if (*cursor == ' ' || *cursor == '\r' || *cursor == '\t') {
            ++cursor;
            ++column;
            continue;
        }

        if (*cursor == '\n') {
            ++cursor;
            ++line;
            column = 1;
            continue;
        }

        if (*cursor == '/' && cursor[1] == '/') {
            cursor += 2;
            column += 2;
            while (*cursor != '\0' && *cursor != '\n') {
                ++cursor;
                ++column;
            }
            continue;
        }

        if (*cursor == '/' && cursor[1] == '*') {
            cursor += 2;
            column += 2;
            while (*cursor != '\0' && !(*cursor == '*' && cursor[1] == '/')) {
                if (*cursor == '\n') {
                    ++cursor;
                    ++line;
                    column = 1;
                } else {
                    ++cursor;
                    ++column;
                }
            }
            if (*cursor == '\0') {
                *error_message = format_source_diagnostic(
                    "Lexer error",
                    source,
                    token_line,
                    token_column,
                    2,
                    "unterminated block comment"
                );
                token_array_free(tokens);
                return 0;
            }
            cursor += 2;
            column += 2;
            continue;
        }

        switch (*cursor) {
            case '(':
                push_token(tokens, TOKEN_LEFT_PAREN, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case ')':
                push_token(tokens, TOKEN_RIGHT_PAREN, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '{':
                push_token(tokens, TOKEN_LEFT_BRACE, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '}':
                push_token(tokens, TOKEN_RIGHT_BRACE, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '[':
                push_token(tokens, TOKEN_LEFT_BRACKET, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case ']':
                push_token(tokens, TOKEN_RIGHT_BRACKET, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case ',':
                push_token(tokens, TOKEN_COMMA, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '.':
                push_token(tokens, TOKEN_DOT, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case ':':
                push_token(tokens, TOKEN_COLON, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case ';':
                push_token(tokens, TOKEN_SEMICOLON, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '+':
                push_token(tokens, TOKEN_PLUS, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '-':
                push_token(tokens, TOKEN_MINUS, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '*':
                push_token(tokens, TOKEN_STAR, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '/':
                push_token(tokens, TOKEN_SLASH, token_start, 1U, 0.0, token_line, token_column);
                ++cursor; ++column;
                continue;
            case '&':
                if (cursor[1] == '&') {
                    push_token(tokens, TOKEN_AND_AND, token_start, 2U, 0.0, token_line, token_column);
                    cursor += 2;
                    column += 2;
                    continue;
                }
                break;
            case '|':
                if (cursor[1] == '|') {
                    push_token(tokens, TOKEN_OR_OR, token_start, 2U, 0.0, token_line, token_column);
                    cursor += 2;
                    column += 2;
                    continue;
                }
                break;
            case '!':
                if (cursor[1] == '=') {
                    push_token(tokens, TOKEN_BANG_EQUAL, token_start, 2U, 0.0, token_line, token_column);
                    cursor += 2;
                    column += 2;
                } else {
                    push_token(tokens, TOKEN_BANG, token_start, 1U, 0.0, token_line, token_column);
                    ++cursor; ++column;
                }
                continue;
            case '=':
                if (cursor[1] == '=') {
                    push_token(tokens, TOKEN_EQUAL_EQUAL, token_start, 2U, 0.0, token_line, token_column);
                    cursor += 2;
                    column += 2;
                } else {
                    push_token(tokens, TOKEN_EQUAL, token_start, 1U, 0.0, token_line, token_column);
                    ++cursor; ++column;
                }
                continue;
            case '>':
                if (cursor[1] == '=') {
                    push_token(tokens, TOKEN_GREATER_EQUAL, token_start, 2U, 0.0, token_line, token_column);
                    cursor += 2;
                    column += 2;
                } else {
                    push_token(tokens, TOKEN_GREATER, token_start, 1U, 0.0, token_line, token_column);
                    ++cursor; ++column;
                }
                continue;
            case '<':
                if (cursor[1] == '=') {
                    push_token(tokens, TOKEN_LESS_EQUAL, token_start, 2U, 0.0, token_line, token_column);
                    cursor += 2;
                    column += 2;
                } else {
                    push_token(tokens, TOKEN_LESS, token_start, 1U, 0.0, token_line, token_column);
                    ++cursor; ++column;
                }
                continue;
            case '"':
                if (!lex_string_literal(source, &cursor, &column, tokens, error_message, token_line, token_column, '"')) {
                    return 0;
                }
                continue;
            case '\'':
                if (!lex_string_literal(source, &cursor, &column, tokens, error_message, token_line, token_column, '\'')) {
                    return 0;
                }
                continue;
            default:
                break;
        }

        if (isdigit((unsigned char)*cursor)) {
            while (isdigit((unsigned char)*cursor)) {
                ++cursor;
                ++column;
            }
            if (*cursor == '.' && isdigit((unsigned char)cursor[1])) {
                ++cursor;
                ++column;
                while (isdigit((unsigned char)*cursor)) {
                    ++cursor;
                    ++column;
                }
            }
            {
                size_t length = (size_t)(cursor - token_start);
                char *number_text = jsrt_malloc(length + 1U);
                memcpy(number_text, token_start, length);
                number_text[length] = '\0';
                push_token(tokens, TOKEN_NUMBER, token_start, length, strtod(number_text, NULL), token_line, token_column);
                free(number_text);
            }
            continue;
        }

        if (isalpha((unsigned char)*cursor) || *cursor == '_') {
            while (isalnum((unsigned char)*cursor) || *cursor == '_') {
                ++cursor;
                ++column;
            }
            {
                size_t length = (size_t)(cursor - token_start);
                char *text = jsrt_malloc(length + 1U);
                memcpy(text, token_start, length);
                text[length] = '\0';
                push_token(tokens, identifier_type(text), text, length, 0.0, token_line, token_column);
                free(text);
            }
            continue;
        }

        {
            char *message = jsrt_format("unexpected character '%c'", *cursor);
            *error_message = format_source_diagnostic("Lexer error", source, token_line, token_column, 1, message);
            free(message);
        }
        token_array_free(tokens);
        return 0;
    }

    push_token(tokens, TOKEN_EOF, "", 0U, 0.0, line, column);
    return 1;
}
