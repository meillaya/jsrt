# Test262 Subset Manifest and Skip/XFail Taxonomy (P6)

Status: accepted on 2026-05-15.

The curated Test262 subset is governed by `conformance/test262-subset-manifest.csv`.

## Manifest columns

`id,feature,milestone,tier,source,local_path,backends,expect,stdout,stderr_contains,skip_reason,xfail_reason`

- `tier`: `supported`, `experimental`, `future`, or `rejected`
- `backends`: `tree`, `vm`, or `both`
- `expect`: `success`, `failure`, `skip`, or `xfail`
- `skip_reason`: required for `skip`
- `xfail_reason`: required for `xfail`

## Taxonomy

- **skip**: not runnable because harness support is absent or the feature is outside the current support tier.
- **xfail**: intentionally tracked known failure for a feature accepted into the bounded production subset.
- **future**: planned but not a current compatibility claim.
- **rejected**: deliberately out of scope; a negative diagnostic case may still be retained.

## Promotion rule

A case cannot move to `supported` without a matching support-tier matrix update, implementation/diagnostic evidence, and passing target in the closure record.
