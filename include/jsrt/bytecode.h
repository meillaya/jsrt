#ifndef JSRT_BYTECODE_H
#define JSRT_BYTECODE_H

#include <stddef.h>

#include "jsrt/value.h"

typedef enum {
    BC_CONST,
    BC_NULL,
    BC_TRUE,
    BC_FALSE,
    BC_POP,
    BC_DUP,
    BC_LOAD_LOCAL,
    BC_STORE_LOCAL,
    BC_LOAD_GLOBAL,
    BC_STORE_GLOBAL,
    BC_DEFINE_GLOBAL,
    BC_NEG,
    BC_NOT,
    BC_ADD,
    BC_SUB,
    BC_MUL,
    BC_DIV,
    BC_EQ,
    BC_NEQ,
    BC_LT,
    BC_LTE,
    BC_GT,
    BC_GTE,
    BC_JUMP,
    BC_JUMP_IF_FALSE,
    BC_LOOP,
    BC_CALL,
    BC_MAKE_FUNCTION,
    BC_RETURN
} BytecodeOp;

typedef struct {
    BytecodeOp op;
    int operand;
} BytecodeInstruction;

typedef struct {
    char *name;
    size_t arity;
    size_t local_count;
    BytecodeInstruction *instructions;
    size_t instruction_count;
    size_t instruction_capacity;
    Value *constants;
    size_t constant_count;
    size_t constant_capacity;
} BytecodeFunction;

typedef struct {
    BytecodeFunction *functions;
    size_t function_count;
    size_t function_capacity;
    char **globals;
    size_t global_count;
    size_t global_capacity;
} BytecodeProgram;

void bytecode_program_init(BytecodeProgram *program);
void bytecode_program_free(BytecodeProgram *program);
char *bytecode_disassemble_program(const BytecodeProgram *program);

#endif
