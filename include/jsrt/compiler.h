#ifndef JSRT_COMPILER_H
#define JSRT_COMPILER_H

#include <stdbool.h>

#include "jsrt/ast.h"
#include "jsrt/bytecode.h"

bool compile_program(const StmtArray *program, const char *source, BytecodeProgram *out_program, char **error_message);

#endif
