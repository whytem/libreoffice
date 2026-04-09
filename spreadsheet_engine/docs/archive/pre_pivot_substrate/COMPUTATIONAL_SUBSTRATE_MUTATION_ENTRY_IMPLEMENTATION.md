# Computational Substrate Mutation Entry Implementation

Status: completed implementation note

## Purpose

This note records the first engine-owned admitted-slice mutation-entry
implementation for
[COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md).

The goal of this workstream is narrower than broad mutation-host transfer:

- introduce an explicit engine-owned admitted mutation request shape
- route admitted requests into authority, lifecycle, or structural paths in
  one place
- build admitted after-state transitions from that engine-owned request
  surface
- update the mutable substrate from those unified mutation-entry transitions
  before any Calc apply/realization layer consumes them

## Landed Engine-Owned Entry Surface

The new engine-owned request and dispatch surface lives in
[MutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutationEntry.hxx).

The landed surface is:

- `MutationEntryRequest`
- `MutationEntryBuildInput`
- `MutationEntryTransition`
- `MutationEntryPath`
- `buildMutationEntryTransition(...)`
- `applyMutableMutationEntryTransition(...)`
- helpers for reading the resulting computational, graph, IR, and recalc
  outputs

This makes admitted mutation entry explicit rather than leaving request
classification spread across separate authority, lifecycle, and structural
call sites.

## Routing Model

The landed routing model is:

- scalar-value requests route to the authority path
- formula insert/replace requests route to the lifecycle path
- `ClearCell` routes by the admitted before-shape:
  - formula cell -> lifecycle
  - non-formula cell -> authority
- admitted row/column structural requests route to the structural path

This is still a bounded admitted-slice implementation, but mutation-entry
intent is now explicitly owned by the engine-facing request surface.

## Mutable Substrate Integration

The mutable substrate now has one unified entry-dispatch helper:

- `applyMutableMutationEntryTransition(...)`

That helper reuses the already-proven:

- authority transition application
- lifecycle transition application
- structural transition application

but moves the dispatch boundary to the engine-owned mutation-entry layer.

## Validation Surface

The landed standalone proof lane is in
[computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx).

That lane now verifies:

- admitted request routing for authority, lifecycle, and structural classes
- exact authority-path transition build from the unified mutation-entry
  request
- exact lifecycle-path transition build from the unified mutation-entry
  request
- exact structural-path transition build from the unified mutation-entry
  request
- exact resident cell, formula-lifetime, and wiring updates after applying
  the unified mutation-entry transition into mutable substrate state

## Boundary Kept Intact

This implementation still does not claim:

- direct Calc apply/realization from the unified mutation-entry surface
- rollback migration
- broad mutation-entry migration outside the admitted slice
- shared-group-sensitive or named-range-sensitive mutation entry

Those remain later workstreams in the same plan.
