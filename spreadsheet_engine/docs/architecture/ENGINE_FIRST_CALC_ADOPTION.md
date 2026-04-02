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

## Exit Direction

This program is progressing well when:

- Calc increasingly routes through engine-owned computation helpers by default
- remaining Calc-local logic becomes easier to describe as host-only
- replay and Calc parity stay green without special-case fallback work
- the architecture docs describe a stable engine/host boundary rather than an
  unfinished extraction program
