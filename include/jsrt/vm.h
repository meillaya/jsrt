#ifndef JSRT_VM_H
#define JSRT_VM_H

#include "jsrt/bytecode.h"
#include "jsrt/eval.h"

EvalResult vm_execute_program(const BytecodeProgram *program, const char *source);

#endif
