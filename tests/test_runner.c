#include <stdio.h>

#include "test.h"

int main(void) {
    TestContext ctx = {0, 0};

    run_lexer_tests(&ctx);
    run_parser_tests(&ctx);
    run_eval_tests(&ctx);
    run_diagnostics_tests(&ctx);

    if (ctx.failed != 0) {
        fprintf(stderr, "Tests failed: %d passed, %d failed\n", ctx.passed, ctx.failed);
        return 1;
    }

    printf("Tests passed: %d assertions\n", ctx.passed);
    return 0;
}
