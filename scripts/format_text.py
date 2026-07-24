#!/usr/bin/env python3
"""Normalize trailing whitespace and final newlines in first-party text files."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
SUFFIXES = {".c", ".h", ".S", ".py", ".sh", ".md", ".txt", ".yml", ".yaml"}
for path in ROOT.rglob("*"):
    if not path.is_file() or ".git" in path.parts or path.suffix not in SUFFIXES:
        continue
    text = path.read_text(encoding="utf-8")
    normalized = "\n".join(line.rstrip() for line in text.splitlines()) + "\n"
    if text != normalized: path.write_text(normalized, encoding="utf-8")
