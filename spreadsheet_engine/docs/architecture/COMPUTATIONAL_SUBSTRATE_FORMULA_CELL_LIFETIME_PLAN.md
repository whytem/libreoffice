# Computational Substrate Formula-Cell Lifetime Plan

Status: completed implementation and closeout record

## Purpose

This document defines the next bounded migration step after the completed
[COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md).

That decision closed with the strongest admitted-slice boundary proven so far:

- the engine owns admitted-slice resident cell storage
- the engine owns admitted-slice resident wiring containers
- the engine owns mutable computational state plus graph, wiring, and queue
  decisions on that slice
- Calc can realize admitted live cells and admitted live wiring from
  engine-owned resident state with exact verification
- Calc still owns formula-cell object lifetime, mutation entry, and rollback

The next adjacent concern is therefore no longer resident cell or resident
wiring state. It is formula-cell object lifetime on the admitted slice.

This plan is not a broad document-host transplant. It is the first
implementation plan for making engine-owned admitted formula-cell lifetime the
source of truth on the admitted slice while Calc temporarily remains the
mutation-entry and rollback host.

## Why This Is The Best Next Path

The current project has already proven:

- engine-owned admitted resident cell storage
- engine-owned admitted resident wiring containers
- engine-owned mutable computational state
- engine-owned graph, wiring, and recalc decisions
- exact Calc-side realization of admitted live cells from resident state
- exact Calc-side realization of admitted live wiring from resident state
- exact queue, computational, and graph verification plus explicit rollback

What it has not yet proven is:

- engine-owned formula-cell object lifetime on the admitted slice
- direct engine-owned document mutation entry
- broad object-lifetime migration outside the admitted slice

The shortest path from the current boundary to broader engine-owned live
computational residency is therefore:

1. make admitted formula-cell lifetime engine-owned
2. keep Calc as the temporary mutation-entry and rollback host
3. preserve engine-owned admitted resident cell and wiring state
4. consider mutation-entry migration only after resident state and admitted
   formula-cell lifetime are both proven stable

## Plan Goal

Determine whether `spreadsheet_engine` can safely own formula-cell create,
replace, remove, and retained-identity decisions on the admitted narrow slice
while Calc realizes that lifetime state into live `ScFormulaCell` objects and
continues to perform final verification and rollback.

The goal is to decide one explicit question:

- proceed with engine-owned admitted-slice formula-cell lifetime
- keep formula-cell lifetime validation-only
- or defer formula-cell lifetime again because the realization and
  verification model is not yet stable enough

## Entry Boundary

This plan begins from the currently settled wiring-container boundary:

- the first-stage extraction boundary is complete and stable
- the computational-substrate program closed with a narrow authority result
- the opt-in narrow rollout is complete and stable on the admitted scalar and
  structural slice
- the completed cell-storage residency proof cycle established:
  - engine-owned admitted resident cell storage
  - exact Calc-side cell mirroring from engine-owned state
- the completed wiring-container residency proof cycle established:
  - engine-owned admitted resident wiring containers
  - exact Calc-side live wiring realization from engine-owned state
- Calc still owns:
  - `ScDocument` mutation entry APIs
  - formula-cell object lifetime
  - final rollback

This plan must therefore treat engine-owned formula-cell lifetime as a new
proof surface, not as an already-admitted extension of the current rollout.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, or persistence into the engine
- replace all `ScDocument` object lifetime in one sweep
- migrate direct document mutation entry in the same cycle
- migrate `ScTokenArray` ownership as part of this first lifetime pass
- widen directly into shared-group-sensitive, named-range-sensitive,
  off-sheet, or sheet-wide structural behavior
- weaken exact queue, computational, graph, or rollback requirements
- treat Calc rollback as already migrated

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted formula-cell lifetime
   surface
2. a checked-in schema note for engine-owned formula-cell lifetime records,
   identity, and equivalence rules
3. an implementation note for the engine-owned admitted-slice lifetime store
4. an implementation note for Calc realization of engine-owned formula-cell
   lifetime into live `ScFormulaCell` objects
5. a checked-in evidence note covering exact differential behavior and
   rollback-triggering cases
6. a checked-in decision record saying whether admitted-slice formula-cell
   lifetime:
   - proceeds
   - remains validation-only
   - or is deferred again

## Workstreams

### 1. Freeze The Formula-Cell Lifetime Contract

Freeze the exact admitted formula-cell lifetime surface before any
implementation work begins.

The contract should name:

- the admitted workbook and mutation classes
- the exact formula-cell lifetime operations that may become engine-owned
- retained Calc-owned surfaces that stay host-only in this cycle
- realization and rollback requirements
- what counts as exact success versus immediate defer

Required artifact:

- one checked-in formula-cell lifetime contract note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_CONTRACT.md)

### 2. Define The Engine-Owned Formula-Cell Lifetime Schema

Define the engine-owned live lifetime shape that will act as the source of
truth on the admitted slice.

This schema note should define:

- engine-owned formula-cell lifetime records
- stable admitted identity and retained-order rules
- create, replace, and remove semantics for ordinary scalar formulas
- exact versus normalized-equivalent after-state comparisons
- forbidden Calc-local identity shortcuts

Required artifact:

- one checked-in engine-owned formula-cell lifetime schema note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_SCHEMA.md)

### 3. Build The Engine-Owned Admitted-Slice Lifetime Store

Extend the current resident cell and resident wiring path so the engine owns
admitted formula-cell lifetime as resident state rather than as a retained
Calc responsibility.

This workstream should:

- add an engine-owned lifetime store for the admitted slice
- update authority, lifecycle, and structural transitions so they mutate that
  store incrementally
- preserve the current shadow/bootstrap builders as validation and import
  helpers
- keep the store value-semantic and free of hidden Calc pointer ownership

Required artifact:

- one checked-in implementation note for the engine-owned admitted-slice
  formula-cell lifetime store

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_IMPLEMENTATION.md)

### 4. Build Calc Realization For Engine-Owned Formula-Cell Lifetime

Teach Calc to realize the engine-owned admitted-slice formula-cell lifetime
state into live `ScFormulaCell` objects without reclaiming authority for
lifetime decisions.

This workstream should:

- add narrow compat adapters that materialize or refresh admitted live
  formula-cell objects from engine-owned lifetime state
- preserve the current engine-owned resident cell and resident wiring paths
- keep Calc as the temporary owner of:
  - mutation entry
  - final rollback
- ensure engine-owned lifetime state, not Calc-local reconstruction, is the
  thing deciding the admitted formula-cell after-state

Required artifact:

- one checked-in implementation note for Calc realization of engine-owned
  formula-cell lifetime

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_REALIZATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_REALIZATION.md)

### 5. Freeze Differential Formula-Cell Lifetime Evidence

Run the bounded proof cycle and record the results.

This evidence note should summarize:

- exact same-mutation comparisons between:
  - engine-owned admitted-slice formula-cell lifetime state
  - Calc realized live formula-cell state
  - engine-owned resident cell state
  - engine-owned resident wiring state
  - engine-owned graph and queue state
- rollback-triggering cases
- repair-detected cases
- memory and performance observations
- whether engine-owned formula-cell lifetime is materially cleaner than the
  current resident-state-plus-host-lifetime boundary

Required artifact:

- one checked-in formula-cell lifetime evidence note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_EVIDENCE.md)

### 6. Freeze The Formula-Cell Lifetime Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-owned formula-cell lifetime on the admitted slice
- keep formula-cell lifetime validation-only
- defer formula-cell lifetime again

The decision record must also state the next adjacent concern after this
decision:

- direct admitted-slice mutation entry
- broader dependency-container migration
- or another newly bounded reassessment if the lifetime proof does not hold

Required artifact:

- one checked-in formula-cell lifetime decision record

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md)

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- a new admitted-slice formula-cell lifetime compat adapter under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)

## Recommended Execution Order

The recommended order is:

1. freeze the admitted formula-cell lifetime contract
2. freeze the engine-owned lifetime schema
3. build the engine-owned admitted lifetime store
4. build Calc realization from the engine-owned lifetime store
5. freeze the differential evidence
6. close with an explicit proceed / validation-only / defer decision

## Validation Contract

At minimum, each bounded implementation step should keep the following green:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

If new formula-cell-lifetime-specific proof lanes are added, they should be
made part of this standing contract before closeout.

## Exit Criteria

This plan is complete only if all of the following are true:

- the admitted formula-cell lifetime contract is frozen and respected
- the engine-owned lifetime schema is explicit and stable
- the engine-owned admitted lifetime store is implemented and proven exact on
  the admitted slice
- Calc can realize admitted live formula-cell objects from engine-owned
  lifetime state without reclaiming after-state authority
- exact computational, graph, and queue verification still hold on the
  admitted slice
- rollback and repair-detected behavior remain explicit and green
- the closeout decision says whether admitted-slice formula-cell lifetime:
  - proceeds
  - remains validation-only
  - or is deferred again

The plan is not complete merely because the engine stores more metadata about
formula cells. It is complete only when the admitted live formula-cell
lifetime boundary is either proven, explicitly limited to validation-only, or
explicitly deferred.
