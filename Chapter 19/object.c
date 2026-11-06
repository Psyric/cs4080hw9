#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"

static Obj* objects;

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = objects;
    objects = object;
    return object;
}

static ObjString* allocateString(int length, bool ownsChars) {
    size_t size = sizeof(ObjString);
    if (ownsChars) {
        size += (size_t)length + 1;
    }

    ObjString* string = (ObjString*)allocateObject(size, OBJ_STRING);
    string->length = length;
    string->ownsChars = ownsChars;
    string->chars = ownsChars ? string->data : NULL;
    if (ownsChars) {
        string->data[length] = '\0';
    }
    return string;
}

ObjString* constantString(const char* chars, int length) {
    ObjString* string = allocateString(length, false);
    string->chars = chars;
    return string;
}

ObjString* copyString(const char* chars, int length) {
    ObjString* string = allocateString(length, true);
    memcpy((char*)string->chars, chars, (size_t)length);
    return string;
}

ObjString* concatenateStrings(const ObjString* a, const ObjString* b) {
    if (a->length > INT_MAX - b->length) {
        return NULL;
    }

    int length = a->length + b->length;
    ObjString* string = allocateString(length, true);
    char* chars = (char*)string->chars;
    memcpy(chars, a->chars, (size_t)a->length);
    memcpy(chars + a->length, b->chars, (size_t)b->length);
    return string;
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            ObjString* string = AS_STRING(value);
            fwrite(string->chars, sizeof(char), (size_t)string->length, stdout);
            break;
        }
    }
}

void freeObjects(void) {
    Obj* object = objects;
    while (object != NULL) {
        Obj* next = object->next;
        switch (object->type) {
            case OBJ_STRING: {
                ObjString* string = (ObjString*)object;
                size_t size = sizeof(ObjString);
                if (string->ownsChars) {
                    size += (size_t)string->length + 1;
                }
                reallocate(object, size, 0);
                break;
            }
        }
        object = next;
    }
    objects = NULL;
}