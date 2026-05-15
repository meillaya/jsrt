# Tree/VM Golden Parity Harness (V1)

Status: accepted on 2026-05-15.

`conformance/vm-parity-manifest.csv` lists fixtures that are supported by both the tree-walk semantic oracle and the opt-in VM backend. `scripts/vm-parity.sh` executes each fixture twice and requires matching exit status and stdout. For failing fixtures, stderr must also match.

The harness is intentionally manifest-gated. Unsupported VM surfaces must not be added as parity cases; they belong in negative diagnostics coverage until the relevant VM implementation milestone lands.

Command:

```sh
make vm-parity
```

Promotion rule: adding a language feature to the VM-supported subset requires adding or updating at least one parity fixture, keeping `make vm-parity` green, and linking the evidence from the milestone closure record.
