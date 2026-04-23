#include "jsrt/bytecode.h"

#include <stdlib.h>
#include <string.h>

#include "jsrt/memory.h"

static const char *bytecode_op_name(BytecodeOp op) {
    switch (op) {
        case BC_CONST: return "CONST";
        case BC_NULL: return "NULL";
        case BC_TRUE: return "TRUE";
        case BC_FALSE: return "FALSE";
        case BC_POP: return "POP";
        case BC_DUP: return "DUP";
        case BC_SWAP: return "SWAP";
        case BC_LOAD_LOCAL: return "LOAD_LOCAL";
        case BC_STORE_LOCAL: return "STORE_LOCAL";
        case BC_LOAD_UPVALUE: return "LOAD_UPVALUE";
        case BC_STORE_UPVALUE: return "STORE_UPVALUE";
        case BC_LOAD_GLOBAL: return "LOAD_GLOBAL";
        case BC_STORE_GLOBAL: return "STORE_GLOBAL";
        case BC_DEFINE_GLOBAL: return "DEFINE_GLOBAL";
        case BC_LOAD_THIS: return "LOAD_THIS";
        case BC_NEG: return "NEG";
        case BC_NOT: return "NOT";
        case BC_ADD: return "ADD";
        case BC_SUB: return "SUB";
        case BC_MUL: return "MUL";
        case BC_DIV: return "DIV";
        case BC_EQ: return "EQ";
        case BC_NEQ: return "NEQ";
        case BC_LT: return "LT";
        case BC_LTE: return "LTE";
        case BC_GT: return "GT";
        case BC_GTE: return "GTE";
        case BC_JUMP: return "JUMP";
        case BC_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case BC_LOOP: return "LOOP";
        case BC_MAKE_ARRAY: return "MAKE_ARRAY";
        case BC_MAKE_OBJECT: return "MAKE_OBJECT";
        case BC_GET_INDEX: return "GET_INDEX";
        case BC_SET_INDEX: return "SET_INDEX";
        case BC_NEW_INSTANCE: return "NEW_INSTANCE";
        case BC_CALL: return "CALL";
        case BC_CALL_METHOD: return "CALL_METHOD";
        case BC_CALL_NEW: return "CALL_NEW";
        case BC_MAKE_FUNCTION: return "MAKE_FUNCTION";
        case BC_RETURN: return "RETURN";
    }
    return "UNKNOWN";
}

void bytecode_program_init(BytecodeProgram *program) {
    program->functions = NULL;
    program->function_count = 0;
    program->function_capacity = 0;
    program->globals = NULL;
    program->global_count = 0;
    program->global_capacity = 0;
}

void bytecode_program_free(BytecodeProgram *program) {
    size_t i;
    if (program == NULL) {
        return;
    }

    for (i = 0; i < program->function_count; ++i) {
        BytecodeFunction *function = &program->functions[i];
        size_t j;
        free(function->name);
        free(function->upvalues);
        free(function->instructions);
        for (j = 0; j < function->constant_count; ++j) {
            value_free(function->constants[j]);
        }
        free(function->constants);
    }
    free(program->functions);

    for (i = 0; i < program->global_count; ++i) {
        free(program->globals[i]);
    }
    free(program->globals);

    bytecode_program_init(program);
}

static char *join_owned(char *prefix, const char *suffix) {
    char *joined = jsrt_format("%s%s", prefix, suffix);
    free(prefix);
    return joined;
}

static char *join_both_owned(char *prefix, char *suffix) {
    char *joined = jsrt_format("%s%s", prefix, suffix);
    free(prefix);
    free(suffix);
    return joined;
}

static char *disassemble_instruction(const BytecodeProgram *program, const BytecodeFunction *function, size_t index) {
    const BytecodeInstruction *instruction = &function->instructions[index];
    const char *name = bytecode_op_name(instruction->op);

    switch (instruction->op) {
        case BC_CONST: {
            char *value_text = value_to_string(function->constants[(size_t)instruction->operand]);
            char *line = jsrt_format("%04zu %-14s %d (%s)\n", index, name, instruction->operand, value_text);
            free(value_text);
            return line;
        }
        case BC_LOAD_GLOBAL:
        case BC_STORE_GLOBAL:
        case BC_DEFINE_GLOBAL:
            return jsrt_format(
                "%04zu %-14s %d (%s)\n",
                index,
                name,
                instruction->operand,
                program->globals[(size_t)instruction->operand]
            );
        case BC_LOAD_LOCAL:
        case BC_STORE_LOCAL:
        case BC_LOAD_UPVALUE:
        case BC_STORE_UPVALUE:
        case BC_LOAD_THIS:
        case BC_CALL:
        case BC_CALL_METHOD:
        case BC_CALL_NEW:
        case BC_NEW_INSTANCE:
        case BC_JUMP:
        case BC_JUMP_IF_FALSE:
        case BC_LOOP:
        case BC_MAKE_ARRAY:
        case BC_MAKE_OBJECT:
            return jsrt_format("%04zu %-14s %d\n", index, name, instruction->operand);
        case BC_MAKE_FUNCTION:
            return jsrt_format(
                "%04zu %-14s %d (%s)\n",
                index,
                name,
                instruction->operand,
                program->functions[(size_t)instruction->operand].name != NULL
                    ? program->functions[(size_t)instruction->operand].name
                    : "<anonymous>"
            );
        default:
            return jsrt_format("%04zu %s\n", index, name);
    }
}

char *bytecode_disassemble_program(const BytecodeProgram *program) {
    size_t i;
    char *result = jsrt_strdup("");

    for (i = 0; i < program->function_count; ++i) {
        const BytecodeFunction *function = &program->functions[i];
        size_t j;
        result = join_both_owned(
            result,
            jsrt_format(
                "function %s arity=%zu locals=%zu\n",
                function->name != NULL ? function->name : "<anonymous>",
                function->arity,
                function->local_count
            )
        );
        for (j = 0; j < function->instruction_count; ++j) {
            char *line = disassemble_instruction(program, function, j);
            result = join_owned(result, line);
            free(line);
        }
        if (i + 1U < program->function_count) {
            result = join_owned(result, "\n");
        }
    }

    return result;
}
