#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "object.h"
#include "vm.h"

static void emitConstant(Chunk* chunk, double number) {
    writeConstant(chunk, NUMBER_VAL(number), 1);
}

static double runChunk(Chunk* chunk) {
    Value result;
    assert(interpret(chunk, &result) == INTERPRET_OK);
    assert(IS_NUMBER(result));
    return AS_NUMBER(result);
}

static void testChallenge1(void) {
    Chunk chunk;
    initChunk(&chunk);

    emitConstant(&chunk, 1);
    emitConstant(&chunk, 2);
    writeChunk(&chunk, OP_MULTIPLY, 1);
    emitConstant(&chunk, 3);
    writeChunk(&chunk, OP_ADD, 1);
    writeChunk(&chunk, OP_RETURN, 1);
    assert(runChunk(&chunk) == 5);
    freeChunk(&chunk);

    initChunk(&chunk);
    emitConstant(&chunk, 1);
    emitConstant(&chunk, 2);
    emitConstant(&chunk, 3);
    writeChunk(&chunk, OP_MULTIPLY, 1);
    writeChunk(&chunk, OP_ADD, 1);
    writeChunk(&chunk, OP_RETURN, 1);
    assert(runChunk(&chunk) == 7);
    freeChunk(&chunk);

    initChunk(&chunk);
    emitConstant(&chunk, 3);
    emitConstant(&chunk, 2);
    writeChunk(&chunk, OP_SUBTRACT, 1);
    emitConstant(&chunk, 1);
    writeChunk(&chunk, OP_SUBTRACT, 1);
    writeChunk(&chunk, OP_RETURN, 1);
    assert(runChunk(&chunk) == 0);
    freeChunk(&chunk);

    initChunk(&chunk);
    emitConstant(&chunk, 1);
    emitConstant(&chunk, 2);
    emitConstant(&chunk, 3);
    writeChunk(&chunk, OP_MULTIPLY, 1);
    writeChunk(&chunk, OP_ADD, 1);
    emitConstant(&chunk, 4);
    emitConstant(&chunk, 5);
    writeChunk(&chunk, OP_NEGATE, 1);
    writeChunk(&chunk, OP_DIVIDE, 1);
    writeChunk(&chunk, OP_SUBTRACT, 1);
    writeChunk(&chunk, OP_RETURN, 1);
    double result = runChunk(&chunk);
    assert(result > 7.799999 && result < 7.800001);
    freeChunk(&chunk);

    puts("Challenge 1 passed: all four precedence examples evaluate correctly.");
}

static void testChallenge2(void) {
    Chunk chunk;
    initChunk(&chunk);

    emitConstant(&chunk, 4);
    emitConstant(&chunk, 3);
    emitConstant(&chunk, 0);
    emitConstant(&chunk, 2);
    writeChunk(&chunk, OP_SUBTRACT, 1);
    writeChunk(&chunk, OP_MULTIPLY, 1);
    writeChunk(&chunk, OP_SUBTRACT, 1);
    writeChunk(&chunk, OP_RETURN, 1);
    assert(runChunk(&chunk) == 10);
    freeChunk(&chunk);

    initChunk(&chunk);
    emitConstant(&chunk, 4);
    emitConstant(&chunk, 3);
    emitConstant(&chunk, 2);
    writeChunk(&chunk, OP_NEGATE, 1);
    writeChunk(&chunk, OP_MULTIPLY, 1);
    writeChunk(&chunk, OP_NEGATE, 1);
    writeChunk(&chunk, OP_ADD, 1);
    writeChunk(&chunk, OP_RETURN, 1);
    assert(runChunk(&chunk) == 10);
    freeChunk(&chunk);

    puts("Challenge 2 passed: both equivalent sequences evaluate to 10.");
}

static void testChallenge3(void) {
    Chunk chunk;
    initChunk(&chunk);

    for (int index = 0; index < 1024; index++) {
        emitConstant(&chunk, index);
    }
    writeChunk(&chunk, OP_RETURN, 1);
    assert(runChunk(&chunk) == 1023);
    freeChunk(&chunk);

    puts("Challenge 3 passed: VM grew its stack to execute 1024 live values.");
}

static void testStrings(void) {
    const char literalSource[] = "\"borrowed\"";
    Value result;
    assert(interpretSource(literalSource, &result) == INTERPRET_OK);
    assert(IS_STRING(result));
    assert(!AS_STRING(result)->ownsChars);
    assert(AS_CSTRING(result) == literalSource + 1);
    assert(AS_STRING(result)->length == 8);

    const char concatSource[] = "\"left\" + \" right\"";
    assert(interpretSource(concatSource, &result) == INTERPRET_OK);
    assert(IS_STRING(result));
    assert(AS_STRING(result)->ownsChars);
    assert(AS_CSTRING(result) == AS_STRING(result)->data);
    assert(strcmp(AS_CSTRING(result), "left right") == 0);

    assert(interpretSource("\"same\" == \"same\"", &result) == INTERPRET_OK);
    assert(IS_BOOL(result) && AS_BOOL(result));

    assert(interpretSource("\"a\" + 1", &result) == INTERPRET_RUNTIME_ERROR);
    puts("Strings passed: borrowed literals, inline owned storage, concatenation, and equality.");
}

int main(int argc, char** argv) {
    initVM();
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--all") == 0)) {
        testChallenge1();
        testChallenge2();
        testChallenge3();
        testStrings();
    } else if (argc == 2 && strcmp(argv[1], "--challenge1") == 0) {
        testChallenge1();
    } else if (argc == 2 && strcmp(argv[1], "--challenge2") == 0) {
        testChallenge2();
    } else if (argc == 2 && strcmp(argv[1], "--challenge3") == 0) {
        testChallenge3();
    } else if (argc == 2 && strcmp(argv[1], "--strings") == 0) {
        testStrings();
    } else {
        fprintf(stderr, "Usage: %s [--all|--challenge1|--challenge2|--challenge3|--strings]\n", argv[0]);
        freeVM();
        return EXIT_FAILURE;
    }
    freeVM();
    return EXIT_SUCCESS;
}