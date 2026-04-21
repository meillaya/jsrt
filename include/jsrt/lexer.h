#ifndef JSRT_LEXER_H
#define JSRT_LEXER_H

#include "jsrt/token.h"

int lex_source(const char *source, TokenArray *tokens, char **error_message);

#endif
