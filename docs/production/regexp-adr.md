# RegExp Production ADR (R1-R9)

Status: accepted on 2026-05-15.

## R1 — Supported subset and safety stance

`jsrt` does not claim ECMAScript `RegExp` support today. The current Supported/Experimental surface is the dependency-free `regexMatch(text, pattern)` helper documented in `docs/regex-and-stdlib.md`.

Current tiny pattern semantics:

- literal characters
- `.` wildcard
- `*` for the previous atom
- `^` start anchor
- `$` end anchor

Unsupported: regex literals, `RegExp` objects, alternation, captures, groups, character classes, flags, Unicode classes, lazy quantifiers, lookaround, replacement APIs, and unbounded compatibility claims.

ReDoS stance: future regex work must remain bounded and add fuzz/ReDoS gates before widening pattern support.

## R2-R3 — Parser and bounded engine

The existing helper is a tiny recursive matcher, not a full parser. R2/R3 are closed for the current production subset by documenting the tiny matcher and retaining deterministic helper tests; richer parser/engine work remains Future until implemented with diagnostics and ReDoS gates.

## R4-R8 — RegExp object, captures, classes, flags, integration, literals

All are Future. Regex literals specifically remain unsupported unless a future parser milestone accepts them. Until then, support claims must name `regexMatch`, not ECMAScript `RegExp`.

## R9 — Conformance/fuzz/ReDoS gate

The current gate is documentation plus unit coverage for helper success and diagnostics. Future regex support must add Test262 subset cases and deterministic fuzz/ReDoS fixtures before moving features to Supported.
