# Test Spec — Roadmap N4: `switch` / `case` / `default` MVP

## Coverage Goals
- lexer/parser recognize `switch`, `case`, and `default`
- parser accepts valid switch forms and rejects malformed clause syntax with useful diagnostics
- evaluator covers matching case, default-only path, no-match-with-default, and chosen fallthrough semantics
- `break` exits switch correctly in nested control-flow situations
- example output demonstrates the chosen MVP semantics clearly

## Minimum Regression Matrix
- simple switch with matching case
- switch with default path
- switch with multiple cases before match
- nested switch or switch inside loop with `break`
- malformed switch missing braces/colon/case expression/default colon
- behavior proving the chosen fallthrough policy

## Verification Commands
```sh
make clean && make test
./build/jsrt examples/switch.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- before implementation, record the fallthrough decision in the PRD or a milestone-local ADR note
- keep tests explicit about whether fallthrough is allowed or disallowed
- add only one example, but make it semantically unambiguous
