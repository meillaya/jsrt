#ifndef JSRT_AST_H
#define JSRT_AST_H

#include <stdbool.h>
#include <stddef.h>

#include "jsrt/token.h"

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringArray;

typedef struct {
    Expr **items;
    size_t count;
    size_t capacity;
} ExprArray;

typedef struct {
    char *key;
    Expr *value;
} ObjectProperty;

typedef struct {
    ObjectProperty *items;
    size_t count;
    size_t capacity;
} ObjectPropertyArray;

typedef struct {
    Stmt **items;
    size_t count;
    size_t capacity;
} StmtArray;

/* Parsed program with the lifetime of the environment that evaluated it.
 * Functions hold pointers into these statements, so an environment owns its
 * units until the garbage collector sweeps the environment itself. */
typedef struct AstUnit {
    StmtArray program;
    struct AstUnit *next;
} AstUnit;

typedef struct {
    Expr *value;
    StmtArray statements;
    bool is_default;
} SwitchCase;

typedef struct {
    SwitchCase *items;
    size_t count;
    size_t capacity;
} SwitchCaseArray;

typedef struct {
    char *name;
    StringArray params;
    StmtArray body;
    bool is_constructor;
    int line;
    int column;
} ClassMethod;

typedef struct {
    ClassMethod *items;
    size_t count;
    size_t capacity;
} ClassMethodArray;

typedef struct {
    char *imported_name;
    char *local_name;
} ImportSpecifier;

typedef struct {
    ImportSpecifier *items;
    size_t count;
    size_t capacity;
} ImportSpecifierArray;

typedef enum {
    LITERAL_NUMBER,
    LITERAL_BOOL,
    LITERAL_NULL,
    LITERAL_STRING
} LiteralKind;

typedef struct {
    LiteralKind kind;
    double number;
    bool boolean;
    char *string;
} Literal;

typedef enum {
    EXPR_LITERAL,
    EXPR_IDENTIFIER,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_LOGICAL,
    EXPR_ASSIGN,
    EXPR_CALL,
    EXPR_THIS,
    EXPR_NEW,
    EXPR_FUNCTION,
    EXPR_ARRAY,
    EXPR_OBJECT,
    EXPR_INDEX,
    EXPR_PROPERTY
} ExprKind;

struct Expr {
    ExprKind kind;
    int line;
    int column;
    union {
        Literal literal;
        struct {
            char *name;
        } identifier;
        struct {
            TokenType op;
            Expr *operand;
        } unary;
        struct {
            Expr *left;
            TokenType op;
            Expr *right;
        } binary;
        struct {
            Expr *left;
            TokenType op;
            Expr *right;
        } logical;
        struct {
            Expr *target;
            Expr *value;
        } assign;
        struct {
            Expr *callee;
            ExprArray arguments;
        } call;
        struct {
            Expr *callee;
            ExprArray arguments;
        } new_expr;
        struct {
            char *name;
            StringArray params;
            StmtArray body;
        } function;
        struct {
            ExprArray elements;
        } array;
        struct {
            ObjectPropertyArray properties;
        } object;
        struct {
            Expr *target;
            Expr *index;
        } index;
        struct {
            Expr *target;
            char *name;
        } property;
    } as;
};

typedef enum {
    STMT_EXPR,
    STMT_VAR,
    STMT_BLOCK,
    STMT_IF,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_WHILE,
    STMT_SWITCH,
    STMT_IMPORT,
    STMT_EXPORT,
    STMT_FUNCTION,
    STMT_CLASS,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE
} StmtKind;

struct Stmt {
    StmtKind kind;
    int line;
    int column;
    union {
        struct {
            Expr *expression;
        } expr;
        struct {
            char *name;
            Expr *initializer;
            bool is_const;
        } var;
        struct {
            StmtArray statements;
        } block;
        struct {
            Expr *condition;
            Stmt *then_branch;
            Stmt *else_branch;
        } if_stmt;
        struct {
            Stmt *body;
            Expr *condition;
        } do_while_stmt;
        struct {
            Stmt *init;
            Expr *condition;
            Expr *increment;
            Stmt *body;
        } for_stmt;
        struct {
            Expr *condition;
            Stmt *body;
        } while_stmt;
        struct {
            Expr *expression;
            SwitchCaseArray cases;
        } switch_stmt;
        struct {
            ImportSpecifierArray specifiers;
            char *module_path;
        } import_stmt;
        struct {
            Stmt *declaration;
        } export_stmt;
        struct {
            char *name;
            StringArray params;
            StmtArray body;
        } function;
        struct {
            char *name;
            ClassMethodArray methods;
        } class_stmt;
        struct {
            Expr *value;
        } return_stmt;
    } as;
};

void string_array_init(StringArray *array);
void string_array_push(StringArray *array, char *value);
void string_array_free(StringArray *array);

void expr_array_init(ExprArray *array);
void expr_array_push(ExprArray *array, Expr *expr);
void expr_array_free(ExprArray *array);

void object_property_array_init(ObjectPropertyArray *array);
void object_property_array_push(ObjectPropertyArray *array, ObjectProperty property);
void object_property_array_free(ObjectPropertyArray *array);

void switch_case_array_init(SwitchCaseArray *array);
void switch_case_array_push(SwitchCaseArray *array, SwitchCase switch_case);
void switch_case_array_free(SwitchCaseArray *array);

void class_method_array_init(ClassMethodArray *array);
void class_method_array_push(ClassMethodArray *array, ClassMethod method);
void class_method_array_free(ClassMethodArray *array);

void import_specifier_array_init(ImportSpecifierArray *array);
void import_specifier_array_push(ImportSpecifierArray *array, ImportSpecifier specifier);
void import_specifier_array_free(ImportSpecifierArray *array);

void stmt_array_init(StmtArray *array);
void stmt_array_push(StmtArray *array, Stmt *stmt);
void stmt_array_free(StmtArray *array);

/* Take ownership of program and return a unit that frees it when released.
 * The caller's StmtArray is reset, so a later stmt_array_free is a no-op. */
AstUnit *ast_unit_create(StmtArray *program);
void ast_unit_free_all(AstUnit *units);

Expr *ast_new_literal_number(double value, int line, int column);
Expr *ast_new_literal_bool(bool value, int line, int column);
Expr *ast_new_literal_null(int line, int column);
Expr *ast_new_literal_string(const char *value, int line, int column);
Expr *ast_new_identifier(const char *name, int line, int column);
Expr *ast_new_unary(TokenType op, Expr *operand, int line, int column);
Expr *ast_new_binary(Expr *left, TokenType op, Expr *right, int line, int column);
Expr *ast_new_logical(Expr *left, TokenType op, Expr *right, int line, int column);
Expr *ast_new_assign(Expr *target, Expr *value, int line, int column);
Expr *ast_new_call(Expr *callee, ExprArray arguments, int line, int column);
Expr *ast_new_this(int line, int column);
Expr *ast_new_new(Expr *callee, ExprArray arguments, int line, int column);
Expr *ast_new_function_expr(const char *name, StringArray params, StmtArray body, int line, int column);
Expr *ast_new_array_literal(ExprArray elements, int line, int column);
Expr *ast_new_object_literal(ObjectPropertyArray properties, int line, int column);
Expr *ast_new_index(Expr *target, Expr *index, int line, int column);
Expr *ast_new_property(Expr *target, const char *name, int line, int column);

Stmt *ast_new_expr_stmt(Expr *expression, int line, int column);
Stmt *ast_new_var_stmt(const char *name, Expr *initializer, bool is_const, int line, int column);
Stmt *ast_new_block_stmt(StmtArray statements, int line, int column);
Stmt *ast_new_if_stmt(Expr *condition, Stmt *then_branch, Stmt *else_branch, int line, int column);
Stmt *ast_new_do_while_stmt(Stmt *body, Expr *condition, int line, int column);
Stmt *ast_new_for_stmt(Stmt *init, Expr *condition, Expr *increment, Stmt *body, int line, int column);
Stmt *ast_new_while_stmt(Expr *condition, Stmt *body, int line, int column);
Stmt *ast_new_switch_stmt(Expr *expression, SwitchCaseArray cases, int line, int column);
Stmt *ast_new_import_stmt(ImportSpecifierArray specifiers, const char *module_path, int line, int column);
Stmt *ast_new_export_stmt(Stmt *declaration, int line, int column);
Stmt *ast_new_function_stmt(const char *name, StringArray params, StmtArray body, int line, int column);
Stmt *ast_new_class_stmt(const char *name, ClassMethodArray methods, int line, int column);
Stmt *ast_new_return_stmt(Expr *value, int line, int column);
Stmt *ast_new_break_stmt(int line, int column);
Stmt *ast_new_continue_stmt(int line, int column);

void expr_destroy(Expr *expr);
void stmt_destroy(Stmt *stmt);

#endif
