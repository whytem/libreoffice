# Engine-First Calc Adoption

## Purpose

The compiler extraction, replay promotion, recalc-orchestration extraction,
and execution-shell extraction programs are complete.

The next frontier is to make the extracted engine-owned computation surface the
default path in more production Calc call sites while keeping Calc as the
document and application host.

This is not a new replay or extraction program. It is an adoption and boundary
consolidation program.

## Goals

1. expand Calc's use of engine-owned runtime and compat helpers where replay
   and Calc validation already prove the shared path
2. keep Calc's remaining local logic clearly host-only
3. retire legacy Calc-local execution call paths once the shared path is
   stable
4. protect the zero-fallback promoted replay baseline while authority widens

## In Scope

- preferring engine-owned execution helpers in more Calc call sites
- collapsing legacy Calc-local call paths that duplicate proven shared
  semantics
- tightening adapter boundaries around host-only services
- continuing parity-focused validation as engine-first adoption expands

## Out Of Scope

- changing storage ownership away from Calc
- revisiting the settled recalc queue/invalidation boundary
- moving UI, UNO, import/export, rendering, or threaded/OpenCL backends into
  the engine
- broad rewrites of `ScInterpreter`

## Main Workstreams

### 1. Expand Engine-Owned Execution By Default

Prioritize Calc call sites where shared runtime or compat helpers already
exist and are validated, but the Calc-side entry path is still more local than
it needs to be.

The intent is to make engine-owned execution semantics the normal production
path, not just an extracted helper library.

### 2. Collapse Legacy Adapter Sprawl

Where Calc still reaches equivalent shared behavior through multiple ad hoc
adapter patterns, converge those paths onto smaller, explicit compat seams.

This includes:

- reducing repeated host-to-engine vocabulary translation
- preferring shared helper calls over local branch copies
- keeping document-service lookups concentrated in narrow Calc adapters

### 3. Keep The Host Boundary Explicit

The remaining Calc-owned logic should stay narrow and easy to explain:

- storage mutation and formula-cell lifecycle
- token-container ownership
- document/session and external-reference services
- printer/path/number-format and similar host-only integrations

Any remaining Calc-local execution code should be retained because it is
host-shaped, not because it is simply still waiting to be moved.

### 4. Maintain The Baseline

Every adoption slice should keep the verified boundary green:

- focused Calc Cppunit coverage for touched call sites
- standalone evaluator/runtime coverage for touched helpers
- one-shot `spreadsheetengine_fods_replay_tests --summary`
- diff hygiene checks

## Phased Implementation Approach

### Phase 1. Default-Path Expansion

Status: complete

Route remaining Calc call sites that already have proven shared runtime
semantics onto engine-owned helpers by default. The first landing scope is the
financial add-in surface where Calc can call the same runtime already used by
standalone execution.

Initial landing scope:

- `ACCRINT`
- `DURATION`
- `YIELDMAT`

Validation:

- focused Calc add-in coverage
- standalone direct and compiled formula coverage
- one-shot promoted-corpus replay summary

### Phase 2. Adapter Convergence

Status: complete

Reduce repeated host-to-engine translation patterns so Calc reaches shared
behavior through smaller, clearer adapter seams.

Targets:

- repeated null-date and basis translation in add-ins
- repeated argument normalization around shared date-sensitive helpers
- ad hoc adapter variants that can become one explicit compat vocabulary

Validation:

- focused Calc/add-in unit coverage for touched adapters
- standalone regression coverage for the shared helper surface
- one-shot promoted-corpus replay summary

### Phase 3. Host-Boundary Consolidation

Status: complete

Tighten the remaining Calc-local execution surface until it is clearly
host-only rather than a leftover duplicate implementation.

Targets:

- explicit host-only seams for document/session services
- removal of residual in-scope local copies that now have shared helpers
- doc/status updates that describe the retained Calc surface as host-shaped

Validation:

- Calc Cppunit coverage for touched host-boundary call sites
- zero-fallback promoted replay summary
- diff hygiene checks

### Phase 4. Baseline Lock-In

Status: pending

Turn the zero-fallback promoted replay baseline into a stronger regression
contract so wider engine-first adoption can proceed without silently weakening
the settled boundary.

Targets:

- explicit replay-baseline assertions in standalone test tooling
- doc/status closeout for the adoption program
- stable validation steps for future engine-first slices

Validation:

- one-shot promoted-corpus replay summary with explicit zero-fallback
  expectations
- full focused Calc/standalone validation lane for touched surfaces
- `git diff --check`

## Exit Direction

This program is progressing well when:

- Calc increasingly routes through engine-owned computation helpers by default
- remaining Calc-local logic becomes easier to describe as host-only
- replay and Calc parity stay green without special-case fallback work
- the architecture docs describe a stable engine/host boundary rather than an
  unfinished extraction program
