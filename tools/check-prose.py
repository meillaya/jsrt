#!/usr/bin/env python3
"""Fail on prose that reads like a generated summary.

The rules come from the house style used by the author's NixOS configuration.
They cover every markdown file, shell script, C source, and C header in this
repository, including comments. Data and build output stay out of the walk
because a CSV full of fixture names is not prose.

    tools/check-prose.py            # the whole tree
    tools/check-prose.py README.md docs/MASTER.md
"""

import argparse
import re
import sys
from pathlib import Path

CHECKED_SUFFIXES = {".md", ".sh", ".c", ".h", ".yml", ".yaml"}

SKIP_DIRS = {
    ".git",
    ".zig-cache",
    "build",
    "zig-out",
    "__pycache__",
}

# The loudest signal of machine written text. Matched case insensitively on a
# word boundary.
BANNED_WORDS = [
    "delve",
    "tapestry",
    "meticulous",
    "pivotal",
    "intricate",
    "interplay",
    "underscore",
    "garner",
    "bolster",
    "vibrant",
    "bustling",
    "multifaceted",
    "seamless",
    "commendable",
    "ever-evolving",
    "realm",
    "testament",
    "showcase",
    "foster",
    "unlock",
    "elevate",
    "embark",
    "robust",
    "crucial",
    "essential",
    "profound",
    "nuanced",
    "holistic",
    "myriad",
    "plethora",
    "leverage",
]

BANNED_PHRASES = [
    "it is important to note",
    "it is worth noting",
    "it should be noted",
    "keep in mind that",
    "plays a crucial role",
    "stands as a testament",
    "in the heart of",
    "nestled in",
    "hidden gem",
    "it just works",
    "batteries included",
    "zero config",
    "from the ground up",
    "first-class citizen",
    "game changer",
    "despite these challenges",
    "challenges remain",
    "only time will tell",
    "to be honest",
    "let me be clear",
    "that is the whole point",
    "not just",
    "not only",
]

EM_DASH = "\u2014"


def offenders(path):
    """Every (line number, rule, line) this file breaks."""
    found = []
    for number, line in enumerate(path.read_text(errors="replace").splitlines(), start=1):
        if EM_DASH in line:
            found.append((number, "em dash", line.strip()))

        lowered = line.lower()
        for phrase in BANNED_PHRASES:
            if phrase in lowered:
                found.append((number, f"phrase {phrase!r}", line.strip()))

        for word in BANNED_WORDS:
            if re.search(rf"\b{re.escape(word)}\b", lowered):
                found.append((number, f"word {word!r}", line.strip()))

    return found


def walk(root):
    """Every checkable file under root, minus generated directories."""
    for path in sorted(root.rglob("*")):
        if any(part in SKIP_DIRS for part in path.relative_to(root).parts):
            continue
        if path.is_file() and path.suffix in CHECKED_SUFFIXES:
            yield path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", help="files to check; the whole tree when omitted")
    arguments = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    paths = [Path(p) for p in arguments.paths] if arguments.paths else list(walk(root))
    exempt = {Path(__file__).resolve()}
    paths = [p for p in paths if p.resolve() not in exempt]

    total = 0
    for path in paths:
        for number, rule, line in offenders(path):
            print(f"{path}:{number}: {rule}\n    {line}")
            total += 1

    if total:
        print(f"\n{total} violation(s)", file=sys.stderr)
        sys.exit(1)

    if not paths:
        sys.exit(f"check-prose: no files to check under {root}")

    print(f"{len(paths)} files checked, no violations")


if __name__ == "__main__":
    main()
