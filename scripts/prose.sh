#!/usr/bin/env bash
# House prose gate. Runs tools/check-prose.py over the tree. See that script
# for the exact rule set: banned words, banned phrases, and no em dashes.
set -euo pipefail

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)

exec python3 "$repo_root/tools/check-prose.py"
