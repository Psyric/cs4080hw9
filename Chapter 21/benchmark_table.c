#include <stdio.h>
#include <string.h>
#include <time.h>

#include "object.h"
#include "table.h"
#include "vm.h"

#define BENCHMARK_ENTRIES 20000
#define BENCHMARK_ROUNDS 5

static double elapsedSeconds(clock_t start, clock_t end) {
    return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

static void benchmarkNumericLookups(void) {
    Table table;
    initTable(&table);
    for (int index = 0; index < BENCHMARK_ENTRIES; index++) {
        tableSet(&table, NUMBER_VAL(index), NUMBER_VAL(index * 2));
    }

    Value value;
    clock_t start = clock();
    for (int round = 0; round < BENCHMARK_ROUNDS; round++) {
        for (int index = 0; index < BENCHMARK_ENTRIES; index++) {
            tableGet(&table, NUMBER_VAL(index), &value);
        }
    }
    clock_t end = clock();
    printf("sequential numeric lookup: %.6f s (%d lookups)\n",
           elapsedSeconds(start, end), BENCHMARK_ENTRIES * BENCHMARK_ROUNDS);
    freeTable(&table);
}

static void benchmarkCommonPrefixStrings(void) {
    Table table;
    initTable(&table);
    char chars[64];
    for (int index = 0; index < BENCHMARK_ENTRIES; index++) {
        snprintf(chars, sizeof(chars), "shared-prefix-for-hash-table-key-%08d", index);
        tableSet(&table, OBJ_VAL(copyString(chars, (int)strlen(chars))), NUMBER_VAL(index));
    }

    Value value;
    clock_t start = clock();
    for (int round = 0; round < BENCHMARK_ROUNDS; round++) {
        for (int index = BENCHMARK_ENTRIES - 1; index >= 0; index--) {
            snprintf(chars, sizeof(chars), "shared-prefix-for-hash-table-key-%08d", index);
            tableGet(&table, OBJ_VAL(copyString(chars, (int)strlen(chars))), &value);
        }
    }
    clock_t end = clock();
    printf("common-prefix string lookup: %.6f s (%d lookups)\n",
           elapsedSeconds(start, end), BENCHMARK_ENTRIES * BENCHMARK_ROUNDS);
    freeTable(&table);
}

static void benchmarkDeleteReinsert(void) {
    Table table;
    initTable(&table);
    for (int index = 0; index < BENCHMARK_ENTRIES; index++) {
        tableSet(&table, NUMBER_VAL(index), NUMBER_VAL(index));
    }

    clock_t start = clock();
    for (int round = 0; round < BENCHMARK_ROUNDS; round++) {
        for (int index = 0; index < BENCHMARK_ENTRIES; index += 2) {
            tableDelete(&table, NUMBER_VAL(index));
        }
        for (int index = 0; index < BENCHMARK_ENTRIES; index += 2) {
            tableSet(&table, NUMBER_VAL(index), NUMBER_VAL(index + round));
        }
    }
    clock_t end = clock();
    printf("delete/reinsert churn: %.6f s (%d mutations)\n",
           elapsedSeconds(start, end), BENCHMARK_ENTRIES * BENCHMARK_ROUNDS);
    freeTable(&table);
}

int main(void) {
    initVM();
    benchmarkNumericLookups();
    benchmarkCommonPrefixStrings();
    benchmarkDeleteReinsert();
    freeVM();
    return 0;
}
