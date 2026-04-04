# Computational Substrate Cell Storage Residency Plan

Status: implementation-ready plan

## Purpose

This document defines the next bounded migration step after the completed
[COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_DECISION_RECORD.md).

That decision closed with a meaningful but still narrow result:

- the engine owns mutable sidecar state on the admitted slice
- the engine owns graph and wiring target decisions on that same slice
- Calc can rebuild the admitted live listener, broadcaster, formula-tree, and
  formula-track surface from those engine-owned targets with exact graph
  verification
- Calc still owns physical cell-storage residency and live container
  residency

The next adjacent concern is therefore no longer shadowing or wiring policy.
It is physical cell-storage residency on the admitted slice.

This plan is not a broad storage transplant. It is the first implementation
plan for making engine-resident cell state the source of truth on the
admitted slice while Calc temporarily remains the mirror, mutation-entry, and
rollback host.

## Why This Is The Best Next Path

The current project has already proven:

- engine-owned mutable computational state on the admitted slice
- engine-owned graph and wiring target decisions on that slice
- exact Calc-side replay of those targets
- exact queue, computational, and graph verification plus explicit rollback

What it has not yet proven is:

- engine-owned physical cell-storage residency
- engine-authored live formula-cell object lifetime
- engine-owned broadcaster/listener container residency
- broad storage migration outside the admitted slice

The shortest path from the current boundary to broader engine-owned
computational residency is therefore:

1. make the admitted-slice cell store engine-resident
2. keep Calc as the temporary mirror and mutation-entry host
3. preserve engine-owned graph and wiring target generation
4. move live listener and broadcaster container residency only after the
   engine-resident cell store is proven stable

## Plan Goal

Determine whether `spreadsheet_engine` can safely own physical cell-storage
residency on the admitted narrow slice while Calc mirrors that state into the
retained live document model and continues to perform final verification and
rollback.

The goal is to decide one explicit question:

- proceed with engine-resident admitted-slice cell storage
- keep cell residency validation-only
- or defer cell-storage residency again because the mirror and verification
  model is not yet stable enough

## Entry Boundary

This plan begins from the currently settled storage-and-wiring boundary:

- the first-stage extraction boundary is complete and stable
- the computational-substrate program closed with a narrow authority result
- the opt-in narrow rollout is complete and stable on the admitted scalar and
  structural slice
- the completed storage-and-wiring proof cycle established:
  - engine-owned mutable sidecar state
  - engine-owned graph and wiring target sets
  - exact Calc-side replay of admitted live listener, broadcaster,
    formula-tree, and formula-track surfaces
- Calc still owns:
  - `ScDocument` storage residency
  - document mutation entry APIs
  - formula-cell object lifetime
  - live broadcaster/listener container residency
  - final rollback

This plan must therefore treat engine-resident cell storage as a new proof
surface, not as an already-admitted extension of the current rollout.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, or persistence into the engine
- replace all `ScDocument` storage in one sweep
- migrate live broadcaster/listener container residency in the same cycle
- migrate `ScTokenArray` ownership as part of this first storage-residency
  pass
- widen directly into shared-group-sensitive, named-range-sensitive,
  off-sheet, or sheet-wide structural behavior
- weaken exact queue, computational, graph, or rollback requirements
- treat Calc formula-cell lifetime as already migrated

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted cell-storage residency
   surface
2. a checked-in schema note for engine-resident cell records, identity, and
   equivalence rules
3. an implementation note for the engine-owned admitted-slice cell store
4. an implementation note for Calc mirroring of engine-resident cell state
5. a checked-in evidence note covering exact differential behavior and
   rollback-triggering cases
6. a checked-in decision record saying whether admitted-slice cell residency:
   - proceeds
   - remains validation-only
   - or is deferred again

## Workstreams

### 1. Freeze The Cell-Storage Residency Contract

Freeze the exact admitted storage-residency surface before any implementation
work begins.

The contract should name:

- the admitted workbook and mutation classes
- the exact cell classes that may become engine-resident
- retained Calc-owned surfaces that stay host-only in this cycle
- mirror and rollback requirements
- what counts as exact success versus immediate defer

Required artifact:

- one checked-in cell-storage residency contract note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_CONTRACT.md)

### 2. Define The Engine-Resident Cell Storage Schema

Define the engine-owned storage shape that will act as the source of truth on
the admitted slice.

This schema note should define:

- engine-resident cell record types for ordinary scalar formulas and
  non-formula companions that must stay in sync on the admitted slice
- stable address identity and ordering rules
- formula payload ownership rules
- exact versus normalized-equivalent after-state comparisons
- forbidden Calc-local identity shortcuts

Required artifact:

- one checked-in engine-resident cell storage schema note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_SCHEMA.md)

### 3. Build The Engine-Owned Admitted-Slice Cell Store

Extend the current mutable substrate so the engine owns the admitted-slice
cell store as resident state rather than as a pure sidecar projection.

This workstream should:

- add an engine-owned cell-storage container for the admitted slice
- update lifecycle and structural transitions so they mutate that store
  incrementally
- preserve the current shadow/bootstrap builders as import and validation
  helpers
- keep the store value-semantic and free of hidden Calc pointer ownership

Required artifact:

- one checked-in implementation note for the engine-owned admitted-slice cell
  store

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_IMPLEMENTATION.md)

### 4. Build Calc Mirroring For Engine-Resident Cells

Teach Calc to mirror the engine-resident admitted-slice cell store into the
retained host document model without reclaiming authority for storage
decisions.

This workstream should:

- add narrow compat adapters that materialize or refresh the admitted Calc
  cell surface from engine-resident state
- preserve the current graph and wiring replay path as the engine-owned
  dependency-side authority
- keep Calc as the temporary owner of:
  - mutation entry
  - formula-cell object lifetime
  - live container residency
  - final rollback
- ensure engine state, not Calc-local reconstruction, is the thing deciding
  the admitted cell after-state

Required artifact:

- one checked-in implementation note for Calc mirroring of engine-resident
  cells

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_MIRRORING.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_MIRRORING.md)

### 5. Freeze Differential Cell-Storage Evidence

Run the bounded proof cycle and record the results.

This evidence note should summarize:

- exact same-mutation comparisons between:
  - engine-resident admitted-slice cell state
  - Calc mirrored live cell state
  - engine-owned graph and wiring target state
- rollback-triggering cases
- repair-detected cases
- memory and performance observations
- whether engine-resident cell storage is materially cleaner than the current
  mutable-sidecar-plus-host-storage boundary

Required artifact:

- one checked-in cell-storage residency evidence note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_EVIDENCE.md)

### 6. Freeze The Cell-Storage Residency Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-resident cell storage on the admitted slice
- keep cell residency validation-only
- defer cell-storage residency again

The decision record must also state the next adjacent concern after this
decision:

- live broadcaster/listener container residency on the admitted slice
- broader structural widening on engine-resident cells
- or another bounded storage reassessment before any further migration

Required artifact:

- one checked-in cell-storage residency decision record

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md)

## Target Surfaces

The first implementation sweep should expect to touch:

- engine substrate types under:
  - [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
  - [GraphWiringDelta.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/GraphWiringDelta.hxx)
- Calc compat adapters under:
  - [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/MutableComputationalSubstrate.hxx)
  - [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- the existing narrow rollout and authority harness under:
  - [ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)
  - [ComputationalSubstrateLifecycle.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx)
  - [ComputationalSubstrateStructural.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx)
- Calc differential coverage under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_compile_diff.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_compile_diff.cxx)
- standalone lanes under:
  - [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
  - [computational_graph_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_graph_tests.cxx)
  - [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)
- top-level docs:
  - [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  - [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)

## Recommended Execution Order

Run this plan in the following order:

1. freeze the cell-storage residency contract
2. freeze the engine-resident cell storage schema
3. build the engine-owned admitted-slice cell store
4. build Calc mirroring for engine-resident cells
5. freeze differential cell-storage evidence
6. freeze the cell-storage residency decision

## Validation Contract

The minimum closeout contract should be:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- one-shot `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Any widened validation during implementation is welcome, but those lanes are
the minimum contract for plan closeout.

## Exit Criteria

This plan is complete only when all of the following are true:

- the admitted storage-residency contract is checked in and matches the code
- the engine-resident cell-storage schema is explicit and stable
- the engine owns admitted-slice cell state as resident mutable state rather
  than as a rebuild-only sidecar projection
- Calc can mirror that admitted state into the retained live host model
  without reclaiming storage authority
- exact queue, computational, and graph verification remain green
- rollback remains explicit on divergence
- the replay baseline remains at zero cached fallback
- a checked-in decision record states whether admitted-slice cell-storage
  residency proceeds, stays validation-only, or is deferred again
