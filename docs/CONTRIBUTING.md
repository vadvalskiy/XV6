# Contributing

## Development model

All kernel changes must preserve the single cumulative xv6 source tree under `xv6/`. Do not create a second xv6 copy for a phase, experiment, or proposed fix. Phase-specific explanations belong in `docs/labs/`; executable changes belong in `xv6/`.

## Branches

Use a conventional prefix followed by a concise kebab-case description:

```text
feature/
fix/
docs/
refactor/
test/
chore/
build/
ci/
```

Example:

```text
fix/ticket-lock-owner-exit
```

## Commits

Use the format:

```text
type(scope): concise imperative summary
```

Keep each commit focused and buildable. Do not use fabricated dates or authors. Add `Co-authored-by` trailers only when those people materially participated in the commit.

## Required checks

Before opening a pull request:

```bash
make lint
make test
make verify-build
```

When QEMU is available:

```bash
make smoke CPUS=2
```

For counter or multicore changes:

```bash
make counter-matrix
```

## Source and documentation rules

- retain MIT copyright and license notices from xv6;
- document new system calls in kernel dispatch, user declarations, stubs, and tests;
- include a deterministic regression or invariant check for behavioral changes;
- distinguish compiled, smoke-tested, benchmarked, and historically reported results;
- do not commit generated images, objects, executables, logs, or `dist/` trees;
- update architecture and verification documentation when interfaces or paths move.
