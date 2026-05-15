# Test262 Adapter Spike (P5)

Status: accepted feasibility spike on 2026-05-15.

## Decision

`jsrt` will not claim broad Test262 compliance. The production path is a curated, source-controlled subset adapter that can run selected Test262-style cases after translating their metadata into the repo-local manifest schema.

## Feasible adapter shape

- Keep imported/adapted cases under `conformance/test262-subset/`.
- Track metadata in `conformance/test262-subset-manifest.csv`.
- Reuse the existing `build/jsrt` CLI and backend selector.
- Treat negative syntax/runtime cases as first-class expected failures.
- Require every imported case to map to a support-tier feature and a milestone.

## Non-goals

- vendoring the full Test262 repository
- implementing harness features such as realms, agents, `$262`, async helpers, or full frontmatter semantics in this spike
- claiming pass percentages outside the curated manifest

## Spike result

The adapter is feasible as a thin manifest-driven wrapper over `scripts/conformance.sh`. The first implementation is intentionally a dry-run/metadata validator in `scripts/test262-adapter.sh`; future C-series work may expand it into execution of imported cases once the manifest contains real adapted files.
