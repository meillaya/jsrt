#ifndef JSRT_PARSER_H
#define JSRT_PARSER_H

#include "jsrt/ast.h"
#include "jsrt/token.h"

/* Deepest statement or expression nesting the parser accepts while it builds
 * the tree. The parser recurses on the C stack, so deeper source would crash
 * the host instead of reporting a syntax error. */
#define JSRT_MAX_PARSE_DEPTH 200

/* The parsed tree is measured after parsing. Iterative consumers such as the
 * evaluator and the compiler recurse over the tree, and so does the AST
 * destructor, so a tree deeper than this would overflow the C stack. */
#define JSRT_MAX_AST_DEPTH 512
#define JSRT_MAX_AST_NODES 400000U

/* A single binary, member, or call chain may not grow past this length. Long
 * left leaning chains add one C recursion level per element when the tree is
 * evaluated or destroyed. */
#define JSRT_MAX_EXPR_CHAIN 64

/* Diagnostics are capped so a malformed file cannot spend seconds building
 * megabytes of stderr. */
#define JSRT_MAX_PARSE_ERRORS 10

int parse_tokens(const TokenArray *tokens, const char *source, StmtArray *program, char **error_message);

#endif
