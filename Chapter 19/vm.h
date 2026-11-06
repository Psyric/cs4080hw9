#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM(void);
void freeVM(void);
InterpretResult interpret(Chunk* chunk, Value* result);
InterpretResult interpretSource(const char* source, Value* result);

#endif