# ADR 0003: Branch Merge Strategy & Quality Gates for Modular Architecture

## Status
Accepted (2026-08-01)

## Context
EdgeOS/EdgeC is transitioning from a monolithic development flow to a modular, domain-separated 4-plane architecture. To ensure production-grade stability, zero physical穿帮 (Zero-Mock), contract alignment, and zero regression across the 127 CTest suite, Go services, and Matrix Omnis physical simulation, a strict branch merge protocol and release process must be enforced.

## Decision

### 1. Branch Strategy
- Main development branch: `feature/modular-refactor`
- Feature / Ticket isolation branches: `ticket/T-XXX`
- Release target branch: `main`

### 2. Merge Gate Conditions
Before any merge from `ticket/T-XXX` to `feature/modular-refactor` or from `feature/modular-refactor` to `main`, all of the following gates must pass with 100% success:
1. **Compilation & Contract Alignment**: `./build.sh` completes cleanly with 0 tag inconsistency errors.
2. **CTest Suite**: `./test.sh` passes 127/127 C unit & integration tests.
3. **Go Test Suite**: `cd go && go test ./...` passes with 0 failures.
4. **Physical Matrix Simulation**: `./test.sh --omnis` passes all 10 E2E scenarios.
5. **Repository Hygiene**: `./scripts/check_repo_clean.sh` passes (no tracked ELF, DB, TGZ, or WASM build artifacts).
6. **Boss Protocol & Constitutional Compliance**: Zero Mock values for physical signals, strictly consumer-driven interfaces, no modification to `rtos/` or `modules/hal/`.

### 3. Release Tagging
Upon merging Phase 0 - Phase 6 ticket work to `main`, a annotated git tag `v0.9.0-modular` shall be created to mark the modular refactor milestone.

## Consequences
- Every ticket is independently verifiable and traceable on its `ticket/T-XXX` branch.
- Zero untracked binary pollution in git history.
- High confidence in production deployments across all 4 planes (Foundation, Control, Compute, Execution).
