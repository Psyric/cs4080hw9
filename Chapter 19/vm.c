#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value* stack;
    int stackCount;
    int stackCapacity;
} VM;

static VM vm;

static void resetStack(void) {
    vm.stackCount = 0;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = getLine(vm.chunk, (int)instruction);
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

static bool push(Value value) {
    if (vm.stackCount == vm.stackCapacity) {
        if (vm.stackCapacity > INT_MAX / 2) {
            fprintf(stderr, "Runtime error: value stack is too large.\n");
            return false;
        }
        int oldCapacity = vm.stackCapacity;
        vm.stackCapacity = GROW_CAPACITY(oldCapacity);
        vm.stack = GROW_ARRAY(Value, vm.stack, oldCapacity, vm.stackCapacity);
    }
    vm.stack[vm.stackCount++] = value;
    return true;
}

static bool pop(Value* value) {
    if (vm.stackCount == 0) {
        fprintf(stderr, "Runtime error: value stack underflow.\n");
        return false;
    }
    *value = vm.stack[--vm.stackCount];
    return true;
}

static Value peek(int distance) {
    return vm.stack[vm.stackCount - 1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

void initVM(void) {
    vm.chunk = NULL;
    vm.ip = NULL;
    vm.stack = NULL;
    vm.stackCount = 0;
    vm.stackCapacity = 0;
}

void freeVM(void) {
    freeObjects();
    FREE_ARRAY(Value, vm.stack, vm.stackCapacity);
    initVM();
}

static InterpretResult run(Value* result) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        Value b; \
        if (!pop(&b)) return INTERPRET_RUNTIME_ERROR; \
        Value a; \
        if (!pop(&a)) return INTERPRET_RUNTIME_ERROR; \
        if (!push(valueType(AS_NUMBER(a) op AS_NUMBER(b)))) return INTERPRET_RUNTIME_ERROR; \
    } while (false)

    while ((size_t)(vm.ip - vm.chunk->code) < (size_t)vm.chunk->count) {
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
            case OP_CONSTANT: {
                if ((size_t)(vm.ip - vm.chunk->code) >= (size_t)vm.chunk->count) {
                    fprintf(stderr, "Runtime error: missing constant operand.\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!push(READ_CONSTANT())) return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_CONSTANT_LONG: {
                if ((size_t)(vm.ip - vm.chunk->code) + 3 > (size_t)vm.chunk->count) {
                    fprintf(stderr, "Runtime error: missing long constant operand.\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint32_t constant = READ_BYTE();
                constant |= (uint32_t)READ_BYTE() << 8;
                constant |= (uint32_t)READ_BYTE() << 16;
                if (constant >= (uint32_t)vm.chunk->constants.count) {
                    fprintf(stderr, "Runtime error: constant index out of range.\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!push(vm.chunk->constants.values[constant])) return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_NIL:
                if (!push(NIL_VAL)) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_TRUE:
                if (!push(BOOL_VAL(true))) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_FALSE:
                if (!push(BOOL_VAL(false))) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_EQUAL: {
                Value b;
                if (!pop(&b)) return INTERPRET_RUNTIME_ERROR;
                Value a;
                if (!pop(&a)) return INTERPRET_RUNTIME_ERROR;
                if (!push(BOOL_VAL(valuesEqual(a, b)))) return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    Value b;
                    if (!pop(&b)) return INTERPRET_RUNTIME_ERROR;
                    Value a;
                    if (!pop(&a)) return INTERPRET_RUNTIME_ERROR;
                    ObjString* resultString = concatenateStrings(AS_STRING(a), AS_STRING(b));
                    if (resultString == NULL) {
                        runtimeError("String is too large.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    if (!push(OBJ_VAL(resultString))) return INTERPRET_RUNTIME_ERROR;
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    BINARY_OP(NUMBER_VAL, +);
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case OP_MULTIPLY:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case OP_NOT: {
                Value value;
                if (!pop(&value)) return INTERPRET_RUNTIME_ERROR;
                if (!push(BOOL_VAL(isFalsey(value)))) return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value operand;
                if (!pop(&operand)) return INTERPRET_RUNTIME_ERROR;
                if (!push(NUMBER_VAL(-AS_NUMBER(operand)))) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_RETURN:
                if (!pop(result)) return INTERPRET_RUNTIME_ERROR;
                return INTERPRET_OK;
            default:
                fprintf(stderr, "Runtime error: unknown opcode %u.\n", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }

    fprintf(stderr, "Runtime error: chunk ended without OP_RETURN.\n");
    return INTERPRET_RUNTIME_ERROR;

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(Chunk* chunk, Value* result) {
    resetStack();
    vm.chunk = chunk;
    vm.ip = chunk->code;
    return run(result);
}

InterpretResult interpretSource(const char* source, Value* result) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    InterpretResult status = interpret(&chunk, result);
    freeChunk(&chunk);
    return status;
}