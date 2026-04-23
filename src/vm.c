#include "jsrt/vm.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/diagnostics.h"
#include "jsrt/env.h"
#include "jsrt/gc.h"
#include "jsrt/memory.h"

typedef enum {
    VM_BUILTIN_PRINT,
    VM_BUILTIN_LEN,
    VM_BUILTIN_TYPEOF,
    VM_BUILTIN_SET_PROTOTYPE_OF
} VmBuiltinKind;

typedef struct {
    JsFunction *closure;
    const BytecodeFunction *function;
    size_t ip;
    size_t call_base;
    size_t base;
    Value this_value;
    bool is_constructor;
    Value constructor_instance;
} VmFrame;

typedef struct {
    const char *source;
    Env *globals;
    Value *stack;
    size_t stack_count;
    size_t stack_capacity;
    VmFrame *frames;
    size_t frame_count;
    size_t frame_capacity;
} Vm;

static EvalResult vm_result_ok(Value value) {
    EvalResult result;
    result.had_error = false;
    result.is_return = false;
    result.is_break = false;
    result.is_continue = false;
    result.value = value;
    result.runtime_error_kind = RUNTIME_ERROR_NONE;
    result.error_line = 0;
    result.error_column = 0;
    result.error_width = 0;
    result.error_message = NULL;
    return result;
}

static const char *runtime_error_kind_name(RuntimeErrorKind kind) {
    switch (kind) {
        case RUNTIME_ERROR_NONE:
            return "none";
        case RUNTIME_ERROR_TYPE:
            return "type";
        case RUNTIME_ERROR_NAME:
            return "name";
        case RUNTIME_ERROR_ARITY:
            return "arity";
        case RUNTIME_ERROR_LOOKUP:
            return "lookup";
        case RUNTIME_ERROR_ASSIGNMENT:
            return "assignment";
        case RUNTIME_ERROR_CONTROL_FLOW:
            return "control-flow";
        case RUNTIME_ERROR_INDEX:
            return "index";
        case RUNTIME_ERROR_MATH:
            return "math";
        case RUNTIME_ERROR_INTERNAL:
            return "internal";
    }
    return "unknown";
}

static EvalResult vm_runtime_error(const Vm *vm, RuntimeErrorKind kind, const char *message) {
    EvalResult result = vm_result_ok(value_null());
    char *label = jsrt_format("Runtime error [%s]", runtime_error_kind_name(kind));

    result.had_error = true;
    result.runtime_error_kind = kind;
    result.error_line = 1;
    result.error_column = 1;
    result.error_width = 1;
    result.error_message = format_source_diagnostic(label, vm->source, 1, 1, 1, message);
    free(label);
    return result;
}

static EvalResult vm_runtime_error_take(const Vm *vm, RuntimeErrorKind kind, char *message) {
    EvalResult result = vm_runtime_error(vm, kind, message);
    free(message);
    return result;
}

static JsObject *vm_create_object(size_t count) {
    JsObject *object = jsrt_malloc(sizeof(*object));
    size_t i;
    gc_register_object(&object->gc, GC_OBJECT_OBJECT);
    object->count = count;
    object->items = jsrt_malloc(sizeof(*object->items) * count);
    object->prototype = NULL;
    for (i = 0; i < count; ++i) {
        object->items[i].key = NULL;
        object->items[i].value = value_null();
    }
    return object;
}

static void vm_free_stack(Vm *vm, size_t from_index) {
    size_t i;
    for (i = from_index; i < vm->stack_count; ++i) {
        value_free(vm->stack[i]);
    }
    vm->stack_count = from_index;
}

static void vm_cleanup(Vm *vm) {
    size_t i;
    vm_free_stack(vm, 0U);
    for (i = 0; i < vm->frame_count; ++i) {
        value_free(vm->frames[i].this_value);
        value_free(vm->frames[i].constructor_instance);
    }
    free(vm->stack);
    free(vm->frames);
    if (vm->globals != NULL) {
        env_destroy(vm->globals);
        vm->globals = NULL;
    }
}

static bool vm_push(Vm *vm, Value value) {
    if (vm->stack_count == vm->stack_capacity) {
        size_t new_capacity = vm->stack_capacity == 0 ? 16U : vm->stack_capacity * 2U;
        vm->stack = jsrt_realloc(vm->stack, sizeof(*vm->stack) * new_capacity);
        vm->stack_capacity = new_capacity;
    }
    vm->stack[vm->stack_count++] = value;
    return true;
}

static Value vm_pop(Vm *vm) {
    return vm->stack[--vm->stack_count];
}

static Value *vm_peek(Vm *vm, size_t distance) {
    return &vm->stack[vm->stack_count - 1U - distance];
}

static bool vm_push_frame(
    Vm *vm,
    JsFunction *closure,
    const BytecodeFunction *function,
    size_t call_base,
    size_t base,
    Value this_value,
    bool is_constructor,
    Value constructor_instance
) {
    size_t i;
    if (vm->frame_count == vm->frame_capacity) {
        size_t new_capacity = vm->frame_capacity == 0 ? 8U : vm->frame_capacity * 2U;
        vm->frames = jsrt_realloc(vm->frames, sizeof(*vm->frames) * new_capacity);
        vm->frame_capacity = new_capacity;
    }
    vm->frames[vm->frame_count].closure = closure;
    vm->frames[vm->frame_count].function = function;
    vm->frames[vm->frame_count].ip = 0U;
    vm->frames[vm->frame_count].call_base = call_base;
    vm->frames[vm->frame_count].base = base;
    vm->frames[vm->frame_count].this_value = value_clone(this_value);
    vm->frames[vm->frame_count].is_constructor = is_constructor;
    vm->frames[vm->frame_count].constructor_instance = value_clone(constructor_instance);
    vm->frame_count += 1U;
    for (i = function->arity; i < function->local_count; ++i) {
        vm_push(vm, value_null());
    }
    return true;
}

static JsFunction *vm_create_native_function(const char *name, size_t arity, VmBuiltinKind kind) {
    JsFunction *function = jsrt_malloc(sizeof(*function));
    gc_register_object(&function->gc, GC_OBJECT_FUNCTION);
    function->is_native = true;
    function->declaration = (const struct Stmt *)(size_t)(kind + 1U);
    function->expression = NULL;
    function->closure = NULL;
    function->native_name = name;
    function->native_arity = arity;
    function->bytecode = NULL;
    function->captures = NULL;
    function->capture_count = 0U;
    function->prototype_object = NULL;
    return function;
}

static VmBuiltinKind vm_native_kind(const JsFunction *function) {
    return (VmBuiltinKind)((size_t)function->declaration - 1U);
}

static JsFunction *vm_create_compiled_function(const BytecodeFunction *function) {
    JsFunction *js_function = jsrt_malloc(sizeof(*js_function));
    gc_register_object(&js_function->gc, GC_OBJECT_FUNCTION);
    js_function->is_native = false;
    js_function->declaration = NULL;
    js_function->expression = NULL;
    js_function->closure = NULL;
    js_function->native_name = NULL;
    js_function->native_arity = function->arity;
    js_function->bytecode = function;
    js_function->captures = NULL;
    js_function->capture_count = function->upvalue_count;
    js_function->prototype_object = vm_create_object(0U);
    if (function->upvalue_count > 0U) {
        js_function->captures = jsrt_malloc(sizeof(*js_function->captures) * function->upvalue_count);
        memset(js_function->captures, 0, sizeof(*js_function->captures) * function->upvalue_count);
    }
    return js_function;
}

static const char *vm_value_type_name(Value value) {
    switch (value.type) {
        case VALUE_NUMBER:
            return "number";
        case VALUE_BOOL:
            return "bool";
        case VALUE_NULL:
            return "null";
        case VALUE_STRING:
            return "string";
        case VALUE_FUNCTION:
            return "function";
        case VALUE_ARRAY:
            return "array";
        case VALUE_OBJECT:
            return "object";
    }
    return "unknown";
}

static EvalResult vm_call_native(const Vm *vm, const JsFunction *function, Value *arguments) {
    switch (vm_native_kind(function)) {
        case VM_BUILTIN_PRINT: {
            char *text = value_to_string(arguments[0]);
            printf("%s\n", text);
            free(text);
            return vm_result_ok(value_null());
        }
        case VM_BUILTIN_LEN:
            switch (arguments[0].type) {
                case VALUE_STRING:
                    return vm_result_ok(value_number((double)strlen(arguments[0].as.string)));
                case VALUE_ARRAY:
                    return vm_result_ok(value_number((double)arguments[0].as.array->count));
                case VALUE_OBJECT:
                    return vm_result_ok(value_number((double)arguments[0].as.object->count));
                default:
                    return vm_runtime_error(vm, RUNTIME_ERROR_TYPE, "len() expects a string, array, or object");
            }
        case VM_BUILTIN_TYPEOF:
            return vm_result_ok(value_string(vm_value_type_name(arguments[0])));
        case VM_BUILTIN_SET_PROTOTYPE_OF: {
            JsObject *receiver;
            JsObject *prototype = NULL;

            if (arguments[0].type != VALUE_OBJECT || arguments[0].as.object == NULL) {
                return vm_runtime_error(vm, RUNTIME_ERROR_TYPE, "setPrototypeOf() expects an object receiver");
            }
            if (arguments[1].type != VALUE_NULL &&
                (arguments[1].type != VALUE_OBJECT || arguments[1].as.object == NULL)) {
                return vm_runtime_error(vm, RUNTIME_ERROR_TYPE, "setPrototypeOf() expects an object or null prototype");
            }

            receiver = arguments[0].as.object;
            if (arguments[1].type == VALUE_OBJECT) {
                prototype = arguments[1].as.object;
            }

            if (prototype == receiver) {
                return vm_runtime_error(vm, RUNTIME_ERROR_ASSIGNMENT, "setPrototypeOf() cannot set an object as its own prototype");
            }

            while (prototype != NULL) {
                if (prototype == receiver) {
                    return vm_runtime_error(vm, RUNTIME_ERROR_ASSIGNMENT, "setPrototypeOf() cannot create a prototype cycle");
                }
                prototype = prototype->prototype;
            }

            receiver->prototype = arguments[1].type == VALUE_OBJECT ? arguments[1].as.object : NULL;
            return vm_result_ok(value_clone(arguments[0]));
        }
    }
    return vm_runtime_error(vm, RUNTIME_ERROR_INTERNAL, "unsupported builtin function");
}

static JsArray *vm_create_array(size_t count) {
    JsArray *array = jsrt_malloc(sizeof(*array));
    size_t i;
    gc_register_object(&array->gc, GC_OBJECT_ARRAY);
    array->count = count;
    array->items = jsrt_malloc(sizeof(*array->items) * count);
    for (i = 0; i < count; ++i) {
        array->items[i] = value_null();
    }
    return array;
}

static ptrdiff_t vm_object_find_key(const JsObject *object, const char *key) {
    size_t i;
    for (i = 0; i < object->count; ++i) {
        if (strcmp(object->items[i].key, key) == 0) {
            return (ptrdiff_t)i;
        }
    }
    return -1;
}

static void vm_object_set_key(JsObject *object, const char *key, Value value) {
    ptrdiff_t existing = vm_object_find_key(object, key);
    if (existing >= 0) {
        value_free(object->items[(size_t)existing].value);
        object->items[(size_t)existing].value = value_clone(value);
        return;
    }
    object->items = jsrt_realloc(object->items, sizeof(*object->items) * (object->count + 1U));
    object->items[object->count].key = jsrt_strdup(key);
    object->items[object->count].value = value_clone(value);
    object->count += 1U;
}

static bool vm_object_lookup_key(JsObject *object, const char *key, JsObject **owner, ptrdiff_t *index) {
    JsObject *current = object;

    while (current != NULL) {
        ptrdiff_t current_index = vm_object_find_key(current, key);
        if (current_index >= 0) {
            if (owner != NULL) {
                *owner = current;
            }
            if (index != NULL) {
                *index = current_index;
            }
            return true;
        }
        current = current->prototype;
    }

    return false;
}

static bool vm_is_function_prototype_property(const char *key) {
    return strcmp(key, "prototype") == 0;
}

static EvalResult vm_function_property_error(const Vm *vm) {
    return vm_runtime_error(vm, RUNTIME_ERROR_TYPE, "function properties are limited to 'prototype' in L3");
}

static EvalResult vm_read_function_property(const Vm *vm, const JsFunction *function, const char *key) {
    if (!vm_is_function_prototype_property(key)) {
        return vm_function_property_error(vm);
    }
    if (function->prototype_object == NULL) {
        return vm_result_ok(value_null());
    }
    return vm_result_ok(value_object_take(function->prototype_object));
}

static EvalResult vm_assign_function_property(const Vm *vm, JsFunction *function, const char *key, Value assigned_value) {
    if (!vm_is_function_prototype_property(key)) {
        return vm_function_property_error(vm);
    }
    if (function->is_native) {
        return vm_runtime_error(vm, RUNTIME_ERROR_TYPE, "native functions are not constructors");
    }
    if (assigned_value.type != VALUE_NULL &&
        (assigned_value.type != VALUE_OBJECT || assigned_value.as.object == NULL)) {
        return vm_runtime_error(vm, RUNTIME_ERROR_TYPE, "function prototype must be an object or null");
    }
    function->prototype_object = assigned_value.type == VALUE_OBJECT ? assigned_value.as.object : NULL;
    return vm_result_ok(value_null());
}

static bool vm_value_is_object_like(Value value) {
    return value.type == VALUE_OBJECT || value.type == VALUE_ARRAY || value.type == VALUE_FUNCTION;
}

static void vm_define_builtin(Env *env, const char *name, size_t arity, VmBuiltinKind kind) {
    Value builtin = value_function(vm_create_native_function(name, arity, kind));
    env_define(env, name, builtin, true);
    value_free(builtin);
}

static void vm_define_builtins(Env *env) {
    vm_define_builtin(env, "print", 1U, VM_BUILTIN_PRINT);
    vm_define_builtin(env, "len", 1U, VM_BUILTIN_LEN);
    vm_define_builtin(env, "typeOf", 1U, VM_BUILTIN_TYPEOF);
    vm_define_builtin(env, "setPrototypeOf", 2U, VM_BUILTIN_SET_PROTOTYPE_OF);
}

static EvalResult vm_binary_add(Value left, Value right) {
    if (left.type == VALUE_NUMBER && right.type == VALUE_NUMBER) {
        double value = left.as.number + right.as.number;
        value_free(left);
        value_free(right);
        return vm_result_ok(value_number(value));
    }

    {
        char *left_text = value_to_string(left);
        char *right_text = value_to_string(right);
        char *joined = jsrt_format("%s%s", left_text, right_text);
        free(left_text);
        free(right_text);
        value_free(left);
        value_free(right);
        return vm_result_ok(value_string_take(joined));
    }
}

static bool vm_ensure_number(Value value) {
    return value.type == VALUE_NUMBER;
}

EvalResult vm_execute_program(const BytecodeProgram *program, const char *source) {
    Vm vm;
    EvalResult result = vm_result_ok(value_null());

    memset(&vm, 0, sizeof(vm));
    vm.source = source;
    vm.globals = env_create(NULL);
    vm_define_builtins(vm.globals);

    if (program->function_count == 0U) {
        result = vm_runtime_error(&vm, RUNTIME_ERROR_INTERNAL, "missing script function");
        goto cleanup;
    }

    vm_push_frame(&vm, NULL, &program->functions[0], 0U, 0U, value_null(), false, value_null());

    while (vm.frame_count > 0U) {
        VmFrame *frame = &vm.frames[vm.frame_count - 1U];
        const BytecodeInstruction *instruction;

        if (frame->ip >= frame->function->instruction_count) {
            result = vm_runtime_error(&vm, RUNTIME_ERROR_INTERNAL, "instruction pointer out of bounds");
            goto cleanup;
        }

        instruction = &frame->function->instructions[frame->ip++];

        switch (instruction->op) {
            case BC_CONST:
                vm_push(&vm, value_clone(frame->function->constants[(size_t)instruction->operand]));
                break;
            case BC_NULL:
                vm_push(&vm, value_null());
                break;
            case BC_TRUE:
                vm_push(&vm, value_bool(true));
                break;
            case BC_FALSE:
                vm_push(&vm, value_bool(false));
                break;
            case BC_POP: {
                Value value = vm_pop(&vm);
                value_free(value);
                break;
            }
            case BC_DUP:
                vm_push(&vm, value_clone(*vm_peek(&vm, 0U)));
                break;
            case BC_SWAP: {
                Value top = vm_pop(&vm);
                Value below = vm_pop(&vm);
                vm_push(&vm, top);
                vm_push(&vm, below);
                break;
            }
            case BC_LOAD_LOCAL:
                vm_push(&vm, value_clone(vm.stack[frame->base + (size_t)instruction->operand]));
                break;
            case BC_STORE_LOCAL: {
                Value value = vm_pop(&vm);
                size_t slot = frame->base + (size_t)instruction->operand;
                value_free(vm.stack[slot]);
                vm.stack[slot] = value;
                break;
            }
            case BC_LOAD_UPVALUE:
                if (frame->closure == NULL || (size_t)instruction->operand >= frame->closure->capture_count) {
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_INTERNAL, "invalid upvalue load");
                    goto cleanup;
                }
                vm_push(&vm, value_clone(frame->closure->captures[(size_t)instruction->operand]));
                break;
            case BC_STORE_UPVALUE: {
                Value value = vm_pop(&vm);
                if (frame->closure == NULL || (size_t)instruction->operand >= frame->closure->capture_count) {
                    value_free(value);
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_INTERNAL, "invalid upvalue store");
                    goto cleanup;
                }
                value_free(frame->closure->captures[(size_t)instruction->operand]);
                frame->closure->captures[(size_t)instruction->operand] = value;
                break;
            }
            case BC_LOAD_GLOBAL: {
                Value value;
                if (!env_get(vm.globals, program->globals[(size_t)instruction->operand], &value)) {
                    result = vm_runtime_error_take(
                        &vm,
                        RUNTIME_ERROR_NAME,
                        jsrt_format("undefined variable '%s'", program->globals[(size_t)instruction->operand])
                    );
                    goto cleanup;
                }
                vm_push(&vm, value);
                break;
            }
            case BC_STORE_GLOBAL: {
                Value value = vm_pop(&vm);
                EnvAssignResult assign_result = env_assign(vm.globals, program->globals[(size_t)instruction->operand], value);
                value_free(value);
                if (assign_result != ENV_ASSIGN_OK) {
                    result = vm_runtime_error_take(
                        &vm,
                        assign_result == ENV_ASSIGN_CONST ? RUNTIME_ERROR_ASSIGNMENT : RUNTIME_ERROR_NAME,
                        assign_result == ENV_ASSIGN_CONST
                            ? jsrt_format("cannot assign to const variable '%s'", program->globals[(size_t)instruction->operand])
                            : jsrt_format("undefined variable '%s'", program->globals[(size_t)instruction->operand])
                    );
                    goto cleanup;
                }
                break;
            }
            case BC_DEFINE_GLOBAL: {
                Value value = vm_pop(&vm);
                if (!env_define(vm.globals, program->globals[(size_t)instruction->operand], value, false)) {
                    value_free(value);
                    result = vm_runtime_error_take(
                        &vm,
                        RUNTIME_ERROR_NAME,
                        jsrt_format("redeclaration of '%s' in the same scope", program->globals[(size_t)instruction->operand])
                    );
                    goto cleanup;
                }
                value_free(value);
                break;
            }
            case BC_LOAD_THIS:
                vm_push(&vm, value_clone(frame->this_value));
                break;
            case BC_NEG: {
                Value value = vm_pop(&vm);
                if (!vm_ensure_number(value)) {
                    value_free(value);
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "unary '-' expects a number");
                    goto cleanup;
                }
                value.as.number = -value.as.number;
                vm_push(&vm, value);
                break;
            }
            case BC_NOT: {
                Value value = vm_pop(&vm);
                bool truthy = value_is_truthy(value);
                value_free(value);
                vm_push(&vm, value_bool(!truthy));
                break;
            }
            case BC_ADD: {
                Value right = vm_pop(&vm);
                Value left = vm_pop(&vm);
                result = vm_binary_add(left, right);
                if (result.had_error) {
                    goto cleanup;
                }
                vm_push(&vm, result.value);
                result.value = value_null();
                break;
            }
            case BC_SUB:
            case BC_MUL:
            case BC_DIV:
            case BC_EQ:
            case BC_NEQ:
            case BC_LT:
            case BC_LTE:
            case BC_GT:
            case BC_GTE: {
                Value right = vm_pop(&vm);
                Value left = vm_pop(&vm);

                if ((instruction->op == BC_SUB || instruction->op == BC_MUL || instruction->op == BC_DIV ||
                        instruction->op == BC_LT || instruction->op == BC_LTE || instruction->op == BC_GT || instruction->op == BC_GTE) &&
                    (!vm_ensure_number(left) || !vm_ensure_number(right))) {
                    value_free(left);
                    value_free(right);
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "binary operator expects numbers");
                    goto cleanup;
                }

                switch (instruction->op) {
                    case BC_SUB:
                        vm_push(&vm, value_number(left.as.number - right.as.number));
                        break;
                    case BC_MUL:
                        vm_push(&vm, value_number(left.as.number * right.as.number));
                        break;
                    case BC_DIV:
                        if (right.as.number == 0.0) {
                            value_free(left);
                            value_free(right);
                            result = vm_runtime_error(&vm, RUNTIME_ERROR_MATH, "division by zero");
                            goto cleanup;
                        }
                        vm_push(&vm, value_number(left.as.number / right.as.number));
                        break;
                    case BC_EQ:
                        vm_push(&vm, value_bool(value_equals(left, right)));
                        break;
                    case BC_NEQ:
                        vm_push(&vm, value_bool(!value_equals(left, right)));
                        break;
                    case BC_LT:
                        vm_push(&vm, value_bool(left.as.number < right.as.number));
                        break;
                    case BC_LTE:
                        vm_push(&vm, value_bool(left.as.number <= right.as.number));
                        break;
                    case BC_GT:
                        vm_push(&vm, value_bool(left.as.number > right.as.number));
                        break;
                    case BC_GTE:
                        vm_push(&vm, value_bool(left.as.number >= right.as.number));
                        break;
                    default:
                        break;
                }
                value_free(left);
                value_free(right);
                break;
            }
            case BC_JUMP:
                frame->ip = (size_t)instruction->operand;
                break;
            case BC_JUMP_IF_FALSE:
                if (!value_is_truthy(*vm_peek(&vm, 0U))) {
                    frame->ip = (size_t)instruction->operand;
                }
                break;
            case BC_LOOP:
                frame->ip = (size_t)instruction->operand;
                break;
            case BC_MAKE_ARRAY: {
                size_t count = (size_t)instruction->operand;
                JsArray *array = vm_create_array(count);
                size_t i;
                for (i = count; i > 0; --i) {
                    array->items[i - 1U] = vm_pop(&vm);
                }
                vm_push(&vm, value_array_take(array));
                break;
            }
            case BC_MAKE_OBJECT: {
                size_t count = (size_t)instruction->operand;
                JsObject *object = vm_create_object(count);
                size_t i;
                for (i = count; i > 0; --i) {
                    Value value = vm_pop(&vm);
                    Value key = vm_pop(&vm);
                    if (key.type != VALUE_STRING) {
                        value_free(value);
                        value_free(key);
                        result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "object literal key must be a string");
                        goto cleanup;
                    }
                    object->items[i - 1U].key = jsrt_strdup(key.as.string);
                    object->items[i - 1U].value = value;
                    value_free(key);
                }
                vm_push(&vm, value_object_take(object));
                break;
            }
            case BC_GET_INDEX: {
                Value key = vm_pop(&vm);
                Value target = vm_pop(&vm);
                if (target.type == VALUE_ARRAY) {
                    size_t index;
                    Value result_value;
                    if (key.type != VALUE_NUMBER) {
                        value_free(target);
                        value_free(key);
                        result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "array indexing expects a numeric index");
                        goto cleanup;
                    }
                    index = (size_t)key.as.number;
                    if ((double)index != key.as.number || index >= target.as.array->count) {
                        value_free(target);
                        value_free(key);
                        result = vm_runtime_error(&vm, RUNTIME_ERROR_INDEX, "array index out of bounds");
                        goto cleanup;
                    }
                    result_value = value_clone(target.as.array->items[index]);
                    value_free(target);
                    value_free(key);
                    vm_push(&vm, result_value);
                    break;
                }
                if (target.type == VALUE_OBJECT) {
                    ptrdiff_t idx;
                    JsObject *owner;
                    Value result_value;
                    if (key.type != VALUE_STRING) {
                        value_free(target);
                        value_free(key);
                        result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "object indexing expects a string key");
                        goto cleanup;
                    }
                    if (!vm_object_lookup_key(target.as.object, key.as.string, &owner, &idx)) {
                        value_free(target);
                        result = vm_runtime_error_take(&vm, RUNTIME_ERROR_LOOKUP, jsrt_format("object key '%s' not found", key.as.string));
                        value_free(key);
                        goto cleanup;
                    }
                    result_value = value_clone(owner->items[(size_t)idx].value);
                    value_free(target);
                    value_free(key);
                    vm_push(&vm, result_value);
                    break;
                }
                if (target.type == VALUE_FUNCTION && target.as.function != NULL) {
                    EvalResult property_result;
                    if (key.type != VALUE_STRING) {
                        value_free(target);
                        value_free(key);
                        result = vm_function_property_error(&vm);
                        goto cleanup;
                    }
                    property_result = vm_read_function_property(&vm, target.as.function, key.as.string);
                    value_free(target);
                    value_free(key);
                    if (property_result.had_error) {
                        result = property_result;
                        goto cleanup;
                    }
                    vm_push(&vm, property_result.value);
                    property_result.value = value_null();
                    break;
                }
                value_free(target);
                value_free(key);
                result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "indexing requires an array or object value");
                goto cleanup;
            }
            case BC_SET_INDEX: {
                Value value = vm_pop(&vm);
                Value key = vm_pop(&vm);
                Value target = vm_pop(&vm);
                if (target.type == VALUE_ARRAY) {
                    size_t index;
                    if (key.type != VALUE_NUMBER) {
                        value_free(target);
                        value_free(key);
                        value_free(value);
                        result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "array assignment expects a numeric index");
                        goto cleanup;
                    }
                    index = (size_t)key.as.number;
                    if ((double)index != key.as.number || index >= target.as.array->count) {
                        value_free(target);
                        value_free(key);
                        value_free(value);
                        result = vm_runtime_error(&vm, RUNTIME_ERROR_INDEX, "array assignment index out of bounds");
                        goto cleanup;
                    }
                    value_free(target.as.array->items[index]);
                    target.as.array->items[index] = value_clone(value);
                    value_free(target);
                    value_free(key);
                    vm_push(&vm, value);
                    break;
                }
                if (target.type == VALUE_OBJECT) {
                    if (key.type != VALUE_STRING) {
                        value_free(target);
                        value_free(key);
                        value_free(value);
                        result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "object assignment expects a string key");
                        goto cleanup;
                    }
                    vm_object_set_key(target.as.object, key.as.string, value);
                    value_free(target);
                    value_free(key);
                    vm_push(&vm, value);
                    break;
                }
                if (target.type == VALUE_FUNCTION && target.as.function != NULL) {
                    EvalResult property_result;
                    if (key.type != VALUE_STRING) {
                        value_free(target);
                        value_free(key);
                        value_free(value);
                        result = vm_function_property_error(&vm);
                        goto cleanup;
                    }
                    property_result = vm_assign_function_property(&vm, target.as.function, key.as.string, value);
                    value_free(target);
                    value_free(key);
                    if (property_result.had_error) {
                        value_free(value);
                        result = property_result;
                        goto cleanup;
                    }
                    eval_result_free(&property_result);
                    vm_push(&vm, value);
                    break;
                }
                value_free(target);
                value_free(key);
                value_free(value);
                result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "index assignment requires an array or object value");
                goto cleanup;
            }
            case BC_NEW_INSTANCE: {
                size_t argc = (size_t)instruction->operand;
                size_t callee_index;
                size_t i;
                Value *callee;
                JsObject *instance;

                if (vm.stack_count < argc + 1U) {
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_INTERNAL, "stack underflow during constructor preparation");
                    goto cleanup;
                }

                callee_index = vm.stack_count - argc - 1U;
                callee = &vm.stack[callee_index];
                if (callee->type != VALUE_FUNCTION || callee->as.function == NULL) {
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "new expects a constructor function");
                    goto cleanup;
                }
                if (callee->as.function->is_native) {
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "native functions are not constructors");
                    goto cleanup;
                }

                if (vm.stack_count == vm.stack_capacity) {
                    size_t new_capacity = vm.stack_capacity == 0 ? 16U : vm.stack_capacity * 2U;
                    vm.stack = jsrt_realloc(vm.stack, sizeof(*vm.stack) * new_capacity);
                    vm.stack_capacity = new_capacity;
                }

                for (i = vm.stack_count; i > callee_index + 1U; --i) {
                    vm.stack[i] = vm.stack[i - 1U];
                }
                instance = vm_create_object(0U);
                instance->prototype = callee->as.function->prototype_object;
                vm.stack[callee_index + 1U] = value_object_take(instance);
                vm.stack_count += 1U;
                break;
            }
            case BC_CALL:
            case BC_CALL_METHOD:
            case BC_CALL_NEW: {
                size_t argc = (size_t)instruction->operand;
                size_t extra = instruction->op == BC_CALL ? 1U : 2U;
                size_t callee_index;
                size_t args_index;
                Value *callee;
                Value this_value = value_null();
                Value constructor_instance = value_null();

                if (vm.stack_count < argc + extra) {
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_INTERNAL, "stack underflow during call");
                    goto cleanup;
                }

                callee_index = vm.stack_count - argc - extra;
                args_index = callee_index + extra;
                callee = &vm.stack[callee_index];

                if (instruction->op == BC_CALL_METHOD) {
                    this_value = value_clone(vm.stack[callee_index + 1U]);
                } else if (instruction->op == BC_CALL_NEW) {
                    this_value = value_clone(vm.stack[callee_index + 1U]);
                    constructor_instance = value_clone(vm.stack[callee_index + 1U]);
                }

                if (callee->type != VALUE_FUNCTION || callee->as.function == NULL) {
                    value_free(this_value);
                    value_free(constructor_instance);
                    result = vm_runtime_error(
                        &vm,
                        RUNTIME_ERROR_TYPE,
                        instruction->op == BC_CALL_NEW
                            ? "new expects a constructor function"
                            : "attempted to call a non-function value"
                    );
                    goto cleanup;
                }

                if (instruction->op == BC_CALL_NEW && callee->as.function->is_native) {
                    value_free(this_value);
                    value_free(constructor_instance);
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_TYPE, "native functions are not constructors");
                    goto cleanup;
                }

                if (callee->as.function->is_native) {
                    EvalResult native_result = vm_call_native(&vm, callee->as.function, &vm.stack[args_index]);
                    size_t i;
                    for (i = callee_index; i < vm.stack_count; ++i) {
                        value_free(vm.stack[i]);
                    }
                    vm.stack_count = callee_index;
                    value_free(this_value);
                    value_free(constructor_instance);
                    if (native_result.had_error) {
                        result = native_result;
                        goto cleanup;
                    }
                    vm_push(&vm, native_result.value);
                    native_result.value = value_null();
                    break;
                }

                if (callee->as.function->bytecode == NULL) {
                    value_free(this_value);
                    value_free(constructor_instance);
                    result = vm_runtime_error(&vm, RUNTIME_ERROR_INTERNAL, "invalid compiled function");
                    goto cleanup;
                }
                if (argc != callee->as.function->bytecode->arity) {
                    value_free(this_value);
                    value_free(constructor_instance);
                    result = vm_runtime_error_take(
                        &vm,
                        RUNTIME_ERROR_ARITY,
                        jsrt_format("expected %zu arguments but got %zu", callee->as.function->bytecode->arity, argc)
                    );
                    goto cleanup;
                }
                vm_push_frame(
                    &vm,
                    callee->as.function,
                    callee->as.function->bytecode,
                    callee_index,
                    args_index,
                    this_value,
                    instruction->op == BC_CALL_NEW,
                    constructor_instance
                );
                value_free(this_value);
                value_free(constructor_instance);
                break;
            }
            case BC_MAKE_FUNCTION:
            {
                JsFunction *closure = vm_create_compiled_function(&program->functions[(size_t)instruction->operand]);
                size_t i;
                for (i = 0; i < closure->capture_count; ++i) {
                    BytecodeUpvalue upvalue = closure->bytecode->upvalues[i];
                    if (upvalue.is_local) {
                        closure->captures[i] = value_clone(vm.stack[frame->base + upvalue.index]);
                    } else {
                        if (frame->closure == NULL || upvalue.index >= frame->closure->capture_count) {
                            result = vm_runtime_error(&vm, RUNTIME_ERROR_INTERNAL, "invalid upvalue capture");
                            value_free(value_function(closure));
                            goto cleanup;
                        }
                        closure->captures[i] = value_clone(frame->closure->captures[upvalue.index]);
                    }
                }
                vm_push(&vm, value_function(closure));
                break;
            }
            case BC_RETURN: {
                Value return_value = vm_pop(&vm);

                if (vm.frame_count == 1U) {
                    value_free(frame->this_value);
                    value_free(frame->constructor_instance);
                    vm.frame_count = 0U;
                    vm_free_stack(&vm, 0U);
                    result = vm_result_ok(return_value);
                    goto cleanup;
                } else {
                    size_t caller_base = frame->call_base;
                    Value final_value = return_value;
                    size_t i;
                    if (frame->is_constructor && !vm_value_is_object_like(return_value)) {
                        value_free(final_value);
                        final_value = value_clone(frame->constructor_instance);
                    }
                    for (i = caller_base; i < vm.stack_count; ++i) {
                        value_free(vm.stack[i]);
                    }
                    vm.stack_count = caller_base;
                    value_free(frame->this_value);
                    value_free(frame->constructor_instance);
                    vm.frame_count -= 1U;
                    vm_push(&vm, final_value);
                }
                break;
            }
        }
    }

cleanup:
    vm_cleanup(&vm);
    gc_collect_with_reason(NULL, result.had_error ? NULL : &result.value, "vm-cleanup");
    return result;
}
