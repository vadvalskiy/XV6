SHELL := /bin/bash
XV6_DIR := xv6
CPUS ?= 2
COUNTER_MODE ?= 2

.PHONY: help setup build run test lint verify-build verify smoke \
        counter-matrix-build counter-matrix dist-verify format clean all

help:
	@printf '%s\n' \
	  'make setup                 Check host build and optional runtime dependencies' \
	  'make build                 Build the cumulative xv6 kernel and filesystem image' \
	  'make run CPUS=N            Boot the cumulative kernel in QEMU' \
	  'make test                  Run fast repository regression tests' \
	  'make lint                  Check repository structure and source invariants' \
	  'make verify-build          Clean-build source, dist, and all counter modes' \
	  'make smoke CPUS=N          Boot once and run every Lab 1-4 guest regression' \
	  'make counter-matrix-build  Compile counter modes 0, 1, and 2' \
	  'make counter-matrix        Run 3 counter modes with CPUS=1 and CPUS=4' \
	  'make dist-verify           Rebuild the xv6 dist tree from its own Makefile' \
	  'make clean                 Remove generated build products' \
	  'make verify                Run lint, tests, and complete build verification'

setup:
	@./scripts/check_dependencies.sh

build:
	@$(MAKE) -C $(XV6_DIR) -j2 fs.img xv6.img

run:
	@$(MAKE) -C $(XV6_DIR) CPUS=$(CPUS) qemu

run-nox:
	@$(MAKE) -C $(XV6_DIR) CPUS=$(CPUS) qemu-nox

test:
	@python3 -m unittest -v tests.test_repository

lint:
	@python3 scripts/verify_repository.py

verify-build:
	@./scripts/verify_builds.sh

smoke:
	@python3 scripts/qemu_smoke.py --cpus $(CPUS)

counter-matrix-build:
	@./scripts/run_counter_matrix.sh --build-only

counter-matrix:
	@./scripts/run_counter_matrix.sh

dist-verify:
	@./scripts/verify_builds.sh --dist-only

format:
	@python3 scripts/format_text.py

clean:
	@$(MAKE) -C $(XV6_DIR) clean >/dev/null 2>&1 || true
	@rm -rf $(XV6_DIR)/dist $(XV6_DIR)/dist-test artifacts \
	  __pycache__ scripts/__pycache__ tests/__pycache__

verify: lint test verify-build

all: verify
