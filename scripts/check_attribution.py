#!/usr/bin/env python3
"""Validate attribution on the reconstructed xv6-labs commit range."""
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


def introducing_commit(path: str) -> str:
    output = subprocess.check_output([
        "git", "-C", str(ROOT), "log", "--diff-filter=A", "--format=%H", "--", path
    ]).decode().splitlines()
    if len(output) != 1:
        raise RuntimeError(f"expected exactly one introducing commit for {path}")
    return output[0]


try:
    anchor = introducing_commit(".xv6-labs-base")
    release = introducing_commit(".xv6-labs-release")
except (subprocess.CalledProcessError, RuntimeError) as exc:
    print(f"ATTRIBUTION CHECK FAIL: {exc}", file=sys.stderr)
    raise SystemExit(1)

commits = subprocess.check_output([
    "git", "-C", str(ROOT), "rev-list", "--reverse", f"{anchor}^..{release}"
]).decode().splitlines()
errors: list[str] = []
counts: Counter[str] = Counter()
for sha in commits:
    raw = subprocess.check_output([
        "git", "-C", str(ROOT), "show", "-s", "--format=%an%x1f%ae%x1f%B", sha
    ]).decode("utf-8")
    name, email, body = raw.split("\x1f", 2)
    if PEOPLE.get(name) != email:
        errors.append(f"{sha[:7]} invalid primary author {name} <{email}>")
    else:
        counts[name] += 1
    participants = {name}
    for other, other_email in PEOPLE.items():
        if f"Co-authored-by: {other} <{other_email}>" in body.splitlines():
            participants.add(other)
    if participants != set(PEOPLE):
        errors.append(f"{sha[:7]} does not record all contributors with valid trailers")
if len(commits) != 27:
    errors.append(f"expected 27 reconstructed commits, found {len(commits)}")
if set(counts.values()) != {9}:
    errors.append(f"primary-author distribution is not 9/9/9: {dict(counts)}")
if errors:
    print("ATTRIBUTION CHECK FAIL", file=sys.stderr)
    for error in errors:
        print(f"- {error}", file=sys.stderr)
    raise SystemExit(1)
print(f"ATTRIBUTION CHECK PASS: {len(commits)} commits; primary authors {dict(counts)}")
