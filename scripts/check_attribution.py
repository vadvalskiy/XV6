#!/usr/bin/env python3
"""Validate that each reconstructed commit records all three contributors."""
from __future__ import annotations
from collections import Counter
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[1]
PEOPLE = {
    "Meraj Rastegar": "mragetsars@gmail.com",
    "Meraj PourHosseiny": "meraj.prhosseiny@ut.ac.ir",
    "Ali Sadeghi": "ali.sadeghi.m@ut.ac.ir",
}
raw = subprocess.check_output([
    "git", "-C", str(ROOT), "log", "--format=%H%x1f%an%x1f%ae%x1f%B%x1e"
]).decode("utf-8")
errors: list[str] = []; counts: Counter[str] = Counter(); total = 0
for record in raw.strip("\x1e\n").split("\x1e"):
    if not record.strip(): continue
    sha, name, email, body = record.lstrip("\n").split("\x1f", 3); total += 1
    if PEOPLE.get(name) != email: errors.append(f"{sha[:7]} invalid primary author {name} <{email}>")
    else: counts[name] += 1
    participants = {name}
    for other, other_email in PEOPLE.items():
        if f"Co-authored-by: {other} <{other_email}>" in body: participants.add(other)
    if participants != set(PEOPLE): errors.append(f"{sha[:7]} does not record all contributors")
if len(set(counts.values())) > 1: errors.append(f"primary-author distribution is not balanced: {dict(counts)}")
if errors:
    print("ATTRIBUTION CHECK FAIL", file=sys.stderr)
    for error in errors: print(f"- {error}", file=sys.stderr)
    raise SystemExit(1)
print(f"ATTRIBUTION CHECK PASS: {total} commits; primary authors {dict(counts)}")
