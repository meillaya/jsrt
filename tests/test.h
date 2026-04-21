#ifndef JSRT_TEST_H
#define JSRT_TEST_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int passed;
    int failed;
} TestContext;

#define EXPECT_TRUE(ctx, condition) do { \
    if (condition) { \
        (ctx)->passed++; \
    } else { \
        (ctx)->failed++; \
        fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    } \
} while (0)

#define EXPECT_FALSE(ctx, condition) EXPECT_TRUE((ctx), !(condition))

#define EXPECT_INT_EQ(ctx, actual, expected) do { \
    int actual_value__ = (actual); \
    int expected_value__ = (expected); \
    if (actual_value__ == expected_value__) { \
        (ctx)->passed++; \
    } else { \
        (ctx)->failed++; \
        fprintf(stderr, "Assertion failed at %s:%d: expected %d, got %d\n", __FILE__, __LINE__, expected_value__, actual_value__); \
    } \
} while (0)

#define EXPECT_SIZE_EQ(ctx, actual, expected) do { \
    size_t actual_value__ = (actual); \
    size_t expected_value__ = (expected); \
    if (actual_value__ == expected_value__) { \
        (ctx)->passed++; \
    } else { \
        (ctx)->failed++; \
        fprintf(stderr, "Assertion failed at %s:%d: expected %zu, got %zu\n", __FILE__, __LINE__, expected_value__, actual_value__); \
    } \
} while (0)

#define EXPECT_DOUBLE_EQ(ctx, actual, expected) do { \
    double actual_value__ = (actual); \
    double expected_value__ = (expected); \
    if (fabs(actual_value__ - expected_value__) < 0.000001) { \
        (ctx)->passed++; \
    } else { \
        (ctx)->failed++; \
        fprintf(stderr, "Assertion failed at %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, expected_value__, actual_value__); \
    } \
} while (0)

#define EXPECT_STRING_EQ(ctx, actual, expected) do { \
    const char *actual_value__ = (actual); \
    const char *expected_value__ = (expected); \
    if (strcmp(actual_value__, expected_value__) == 0) { \
        (ctx)->passed++; \
    } else { \
        (ctx)->failed++; \
        fprintf(stderr, "Assertion failed at %s:%d: expected '%s', got '%s'\n", __FILE__, __LINE__, expected_value__, actual_value__); \
    } \
} while (0)

void run_lexer_tests(TestContext *ctx);
void run_parser_tests(TestContext *ctx);
void run_eval_tests(TestContext *ctx);
void run_diagnostics_tests(TestContext *ctx);
void run_bytecode_tests(TestContext *ctx);

#endif
