#!/usr/bin/env python3
"""Normalize maintained repository text without rewriting archived course artifacts."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = {
    ".editorconfig",
    ".gitattributes",
    ".gitignore",
    "Makefile",
    "README.md",
}
FIRST_PARTY_DIRS = {".github", "docs", "scripts", "tests"}
SUFFIXES = {".md", ".py", ".sh", ".txt", ".yml", ".yaml"}
ARCHIVAL_NAMES = {"assignment.fa.txt", "report.fa.md"}


def eligible(path: Path) -> bool:
    """Return whether *path* is maintained first-party text."""
    relative = path.relative_to(ROOT)
    if not path.is_file() or ".git" in relative.parts:
        return False
    if relative.as_posix() in ROOT_FILES:
        return True
    if not relative.parts or relative.parts[0] not in FIRST_PARTY_DIRS:
        return False
    if path.name in ARCHIVAL_NAMES:
        return False
    return path.suffix in SUFFIXES or path.name == "Makefile"


for candidate in ROOT.rglob("*"):
    if not eligible(candidate):
        continue
    text = candidate.read_text(encoding="utf-8")
    normalized = "
".join(line.rstrip() for line in text.splitlines()) + "
"
    if text != normalized:
        candidate.write_text(normalized, encoding="utf-8")
