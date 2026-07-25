#!/usr/bin/env python3
"""Check MRS-RS structure and cumulative xv6 invariants."""
from __future__ import annotations
from pathlib import Path
import re
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[1]
REQUIRED_ROOT = ["README.md", "README.fa.md", ".gitignore", ".gitattributes", ".editorconfig",
                 "Makefile", "LICENSE", "NOTICE.md", "CHANGELOG.md", "CITATION.cff"]
GENERATED_NAMES = {"fs.img", "xv6.img", "xv6memfs.img", "kernel", "kernelmemfs", "bootblock",
                   "entryother", "initcode", "mkfs", "vectors.S"}
GENERATED_SUFFIXES = {".o", ".d", ".asm", ".sym", ".pyc", ".log"}
CONTRIBUTORS = [
    ("Meraj Rastegar", "mragetsars@gmail.com", "mragetsars"),
    ("Meraj PourHosseiny", "meraj.prhosseiny@ut.ac.ir", "MerajPoorhosseiny"),
    ("Ali Sadeghi", "ali.sadeghi.m@ut.ac.ir", "Alisssaaaddd"),
]
EXPECTED_PROGRAMS = {"_find_sum", "_lab1test", "_lab2test", "_schedverify", "_scounttest",
                     "_pctest", "_rwtest", "_tickettest", "_sort_kernel", "_sort_user",
                     "_workload_short", "_workload_long", "_workload_mixed", "_workload_aging"}

def tracked() -> list[Path]:
    output = subprocess.check_output(["git", "-C", str(ROOT), "ls-files", "-z"])
    return [ROOT / item.decode() for item in output.split(b"\0") if item]

def main() -> int:
    errors: list[str] = []
    for name in REQUIRED_ROOT:
        if not (ROOT / name).is_file(): errors.append(f"missing root file: {name}")
    if not (ROOT / "xv6/Makefile").is_file(): errors.append("missing single cumulative xv6 source tree")
    if list(ROOT.glob("labs/*/xv6")): errors.append("parallel per-lab xv6 trees are forbidden")
    for lab in range(1, 5):
        slug = f"lab-{lab:02d}"
        summary = ROOT / "docs" / "labs" / f"{slug}.md"
        assignment = ROOT / "docs" / "assignments" / slug
        report = ROOT / "reports" / slug
        if not summary.is_file():
            errors.append(f"missing Lab {lab} summary: {summary.relative_to(ROOT)}")
        for name in ("assignment.fa.pdf", "assignment.fa.txt"):
            path = assignment / name
            if not path.is_file(): errors.append(f"missing Lab {lab} assignment artifact: {name}")
        for name in ("report.fa.md", "report.fa.pdf"):
            path = report / name
            if not path.is_file(): errors.append(f"missing Lab {lab} report artifact: {name}")
            elif path.stat().st_size == 0: errors.append(f"empty Lab {lab} report artifact: {name}")
    if (ROOT / "README.md").is_file():
        readme = (ROOT / "README.md").read_text()
        if readme.count("\n# ") or not readme.startswith("# Xv6 Operating System Labs\n"):
            errors.append("README must have exactly one expected H1")
        for name, email, login in CONTRIBUTORS:
            for value in (name, email, login):
                if value not in readme: errors.append(f"README missing contributor value: {value}")
    makefile = (ROOT / "xv6/Makefile").read_text()
    for program in EXPECTED_PROGRAMS:
        if program not in makefile: errors.append(f"cumulative UPROGS missing {program}")
    syscall_h = (ROOT / "xv6/syscall.h").read_text()
    numbers = re.findall(r"^#define\s+SYS_\w+\s+(\d+)\s*$", syscall_h, re.M)
    if len(numbers) != len(set(numbers)): errors.append("duplicate syscall numbers")
    key_files = ["find_sum.c", "lab1test.c", "lab2test.c", "schedverify.c", "schedstat.h",
                 "scounttest.c", "pctest.c", "rwtest.c", "tickettest.c"]
    for name in key_files:
        if not (ROOT / "xv6" / name).is_file(): errors.append(f"missing cumulative source: {name}")
    for path in tracked():
        rel = path.relative_to(ROOT)
        if path.name in GENERATED_NAMES or path.suffix in GENERATED_SUFFIXES or "dist" in rel.parts:
            errors.append(f"generated artifact tracked: {rel}")
        if path.is_symlink(): errors.append(f"unexpected symlink: {rel}")
        if path.suffix in {".c", ".h", ".S", ".py", ".sh", ".md", ".txt", ".yml", ".yaml"} or path.name == "Makefile":
            data = path.read_bytes()
            if b"\r\n" in data: errors.append(f"CRLF line endings: {rel}")
            if data and not data.endswith(b"\n"): errors.append(f"missing final newline: {rel}")
    if errors:
        print("REPOSITORY CHECK FAIL", file=sys.stderr)
        for error in sorted(set(errors)): print(f"- {error}", file=sys.stderr)
        return 1
    print(f"REPOSITORY CHECK PASS: {len(tracked())} tracked files")
    return 0
if __name__ == "__main__": raise SystemExit(main())
