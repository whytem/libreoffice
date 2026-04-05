# Computational Substrate Wiring Container Residency Plan

Status: implementation-ready plan

## Purpose

This document defines the next bounded migration step after the completed
[COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md).

That decision closed with a stronger but still narrow boundary:

- the engine owns admitted-slice resident cell storage
- the engine owns mutable computational state on that same slice
- the engine owns graph and wiring target decisions on that same slice
- Calc can mirror admitted live cells from engine-owned resident state and
  rebuild admitted live wiring from engine-owned targets with exact
  verification
- Calc still owns live broadcaster/listener container residency

The next adjacent concern is therefore no longer admitted cell residency. It
is live broadcaster/listener container residency on the admitted slice.

This plan is not a broad dependency-substrate transplant. It is the first
implementation plan for making engine-resident wiring containers the source
of truth on the admitted slice while Calc temporarily remains the mutation-
entry, formula-cell-lifetime, and rollback host.

## Why This Is The Best Next Path

The current project has already proven:

- engine-owned admitted resident cell storage
- engine-owned mutable computational state
- engine-owned graph and wiring target decisions
- exact Calc-side mirroring of admitted live cells
- exact Calc-side replay of admitted live wiring targets
- exact queue, computational, and graph verification plus explicit rollback

What it has not yet proven is:

- engine-owned live broadcaster/listener container residency
- engine-authored formula-cell object lifetime
- broad dependency-container migration outside the admitted slice

The shortest path from the current boundary to broader engine-owned live
dependency residency is therefore:

1. make admitted live wiring containers engine-resident
2. keep Calc as the temporary mutation-entry and formula-cell-lifetime host
3. preserve engine-owned admitted cell residency and graph/wiring target
   generation
4. move formula-cell lifetime only after resident cells and resident wiring
   containers are both proven stable

## Plan Goal

Determine whether `spreadsheet_engine` can safely own live
broadcaster/listener, formula-tree, and formula-track container residency on
the admitted narrow slice while Calc realizes that state through a temporary
mirror/apply path and continues to perform final verification and rollback.

The goal is to decide one explicit question:

- proceed with engine-resident admitted-slice wiring containers
- keep container residency validation-only
- or defer container residency again because the host realization and
  verification model is not yet stable enough

## Entry Boundary

This plan begins from the currently settled cell-storage boundary:

- the first-stage extraction boundary is complete and stable
- the computational-substrate program closed with a narrow authority result
- the opt-in narrow rollout is complete and stable on the admitted scalar and
  structural slice
- the completed storage-and-wiring proof cycle established:
  - engine-owned graph and wiring target sets
  - exact Calc-side replay of admitted live listener, broadcaster,
    formula-tree, and formula-track surfaces
- the completed cell-storage residency proof cycle established:
  - engine-owned admitted resident cell storage
  - exact Calc-side cell mirroring from engine-owned state
- Calc still owns:
  - `ScDocument` mutation entry APIs
  - formula-cell object lifetime
  - live broadcaster/listener container residency
  - final rollback

This plan must therefore treat engine-resident live wiring containers as a
new proof surface, not as an already-admitted extension of the current
rollout.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, or persistence into the engine
- replace all `ScDocument` dependency storage in one sweep
- migrate formula-cell object lifetime in the same cycle
- migrate `ScTokenArray` ownership as part of this first container-residency
  pass
- widen directly into shared-group-sensitive, named-range-sensitive,
  off-sheet, or sheet-wide structural behavior
- weaken exact queue, computational, graph, or rollback requirements
- treat Calc mutation entry as already migrated

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted wiring-container
   residency surface
2. a checked-in schema note for engine-resident wiring nodes, listener edges,
   and realized container identity/equivalence rules
3. an implementation note for the engine-owned admitted-slice wiring
   container store
4. an implementation note for Calc realization or mirroring of
   engine-resident wiring containers
5. a checked-in evidence note covering exact differential behavior and
   rollback-triggering cases
6. a checked-in decision record saying whether admitted-slice wiring
   container residency:
   - proceeds
   - remains validation-only
   - or is deferred again

## Workstreams

### 1. Freeze The Wiring-Container Residency Contract

Freeze the exact admitted wiring-container residency surface before any
implementation work begins.

The contract should name:

- the admitted workbook and mutation classes
- the exact live wiring containers that may become engine-resident
- retained Calc-owned surfaces that stay host-only in this cycle
- realization, mirror, and rollback requirements
- what counts as exact success versus immediate defer

Required artifact:

- one checked-in wiring-container residency contract note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_CONTRACT.md)

### 2. Define The Engine-Resident Wiring Container Schema

Define the engine-owned live wiring shape that will act as the source of
truth on the admitted slice.

This schema note should define:

- engine-resident broadcaster-node records
- engine-resident listener-edge records
- engine-resident formula-tree and formula-track realized order
- stable admitted identity and ordering rules
- exact versus normalized-equivalent after-state comparisons
- forbidden Calc-local identity shortcuts

Required artifact:

- one checked-in engine-resident wiring-container schema note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_SCHEMA.md)

### 3. Build The Engine-Owned Admitted-Slice Wiring Container Store

Extend the current graph/wiring target path so the engine owns admitted live
container residency as resident state rather than as a pure replay target.

This workstream should:

- add an engine-owned container store for the admitted slice
- update lifecycle and structural transitions so they mutate that store
  incrementally
- preserve the current graph-shadow builders as validation and bootstrap
  helpers
- keep the store value-semantic and free of hidden Calc pointer ownership

Required artifact:

- one checked-in implementation note for the engine-owned admitted-slice
  wiring container store

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_IMPLEMENTATION.md)

### 4. Build Calc Realization For Engine-Resident Wiring Containers

Teach Calc to realize the engine-resident admitted-slice wiring container
state into the retained host document model without reclaiming authority for
dependency-container decisions.

This workstream should:

- add narrow compat adapters that materialize or refresh the admitted live
  wiring surface from engine-resident state
- preserve the current engine-owned resident cell-storage path
- keep Calc as the temporary owner of:
  - mutation entry
  - formula-cell object lifetime
  - final rollback
- ensure engine-resident container state, not Calc-local reconstruction, is
  the thing deciding the admitted wiring after-state

Required artifact:

- one checked-in implementation note for Calc realization of engine-resident
  wiring containers

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_REALIZATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_REALIZATION.md)

### 5. Freeze Differential Wiring-Container Evidence

Run the bounded proof cycle and record the results.

This evidence note should summarize:

- exact same-mutation comparisons between:
  - engine-resident admitted-slice wiring container state
  - Calc realized live wiring state
  - engine-owned resident cell state
  - engine-owned graph and queue state
- rollback-triggering cases
- repair-detected cases
- memory and performance observations
- whether engine-resident container residency is materially cleaner than the
  current target-set-plus-host-container boundary

Required artifact:

- one checked-in wiring-container residency evidence note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_EVIDENCE.md)

### 6. Freeze The Wiring-Container Residency Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-resident wiring containers on the admitted slice
- keep container residency validation-only
- defer container residency again

The decision record must also state the next adjacent concern after this
decision:

- formula-cell object lifetime on the admitted slice
- broader structural widening on engine-resident cells and containers
- or another bounded dependency-residency reassessment before any further
  migration

Required artifact:

- one checked-in wiring-container residency decision record

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md)

## Target Surfaces

The first implementation sweep should expect to touch:

- engine substrate types under:
  - [GraphWiringDelta.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/GraphWiringDelta.hxx)
  - [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- Calc compat adapters under:
  - [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
  - [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
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

1. freeze the wiring-container residency contract
2. freeze the engine-resident wiring-container schema
3. build the engine-owned admitted-slice wiring container store
4. build Calc realization for engine-resident wiring containers
5. freeze differential wiring-container evidence
6. freeze the wiring-container residency decision

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

- the admitted wiring-container contract is checked in and matches the code
- the engine-resident wiring-container schema is explicit and stable
- the engine owns admitted-slice live wiring container state as resident
  mutable state rather than as replay-only targets
- Calc can realize that admitted state into the retained live host model
  without reclaiming wiring-container authority
- exact queue, computational, and graph verification remain green
- rollback remains explicit on divergence
- the replay baseline remains at zero cached fallback
- a checked-in decision record states whether admitted-slice wiring-container
  residency proceeds, stays validation-only, or is deferred again
