# L8 Regex and Richer Standard Library

This document records the bounded **L8 — Regex + richer standard library** surface.

## New built-ins

```js
strContains(text, needle);
strSlice(text, start, end);
regexMatch(text, pattern);
```

- `strContains` returns whether `needle` appears in `text`.
- `strSlice` returns the half-open byte slice `[start, end)` and rejects invalid ranges.
- `regexMatch` implements a tiny dependency-free pattern matcher.

## Regex subset

`regexMatch(text, pattern)` supports:

- literal characters
- `.` to match any single character
- `*` to repeat the previous atom zero or more times
- `^` to anchor at the start
- `$` to anchor at the end

Without `^`, matching searches for the pattern anywhere in the text. This is intentionally a teaching subset, not ECMAScript `RegExp`.

Unsupported regex features include character classes, alternation, groups, captures, escapes, flags, Unicode classes, lazy quantifiers, lookaround, replacement, and regex literals.

## Verification evidence

- `tests/test_eval.c` covers string contains, slicing, regex success/failure, and diagnostics.
- `src/eval.c` keeps the implementation dependency-free.
