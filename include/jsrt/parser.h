#ifndef JSRT_PARSER_H
#define JSRT_PARSER_H

#include "jsrt/ast.h"
#include "jsrt/token.h"

int parse_tokens(const TokenArray *tokens, const char *source, StmtArray *program, char **error_message);

#endif
