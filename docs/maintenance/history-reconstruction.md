# Git history reconstruction

## Purpose

The supplied project material contained four laboratory snapshots but did not provide a complete original Git repository that demonstrated the intended serial evolution. The repository history was therefore reconstructed to express the actual dependency model: one xv6 baseline followed by four cumulative phases.

## Integrity rules

- No historical author dates or commit dates were invented.
- Reconstruction commits use the actual reconstruction period on 25 July 2026.
- Source changes are separated into baseline import, phase features, fixes, tests, documentation, build tooling, CI, and release documentation.
- Every commit records all three contributors.
- One contributor is the primary Git author; the other two appear in exact `Co-authored-by` trailers.
- Primary authorship rotates evenly: nine primary commits per contributor across 27 commits.
- Milestone source commits were clean-built during reconstruction.

## Contributor identities

| Contributor | Git author identity | GitHub login |
| --- | --- | --- |
| Meraj Rastegar | `Meraj Rastegar <mragetsars@gmail.com>` | `@mragetsars` |
| Meraj PourHosseiny | `Meraj PourHosseiny <meraj.prhosseiny@ut.ac.ir>` | `@MerajPoorhosseiny` |
| Ali Sadeghi | `Ali Sadeghi <ali.sadeghi.m@ut.ac.ir>` | `@Alisssaaaddd` |

## Commit sequence

```text
01 baseline import
02–06 cumulative Lab 1 implementation, test, and documentation
07–11 cumulative Lab 2 implementation, fix, test, and documentation
12–17 cumulative Lab 3 implementation, fix, test, and documentation
18–24 cumulative Lab 4 implementation, integration fix, test, and documentation
25 repository build and verification tooling
26 continuous integration and full counter matrix
27 MRS-RS release documentation
```

This history is an auditable reconstruction, not a claim that these exact commits or timestamps existed during the original coursework.

## Verification

Run:

```bash
python3 scripts/check_attribution.py
git shortlog -sne HEAD
git log --format=fuller --show-signature
```

The attribution script rejects an unknown primary author, a missing co-author trailer, or an unequal primary-author distribution.
