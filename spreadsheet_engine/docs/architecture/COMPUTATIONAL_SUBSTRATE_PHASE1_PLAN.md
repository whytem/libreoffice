# Computational Substrate Phase 1 Plan

Status: active implementation-ready Phase 1 plan

## Purpose

This document is the execution-ready plan for Phase 1 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

Phase 1 is the first phase that introduces an engine-owned shadow of the live
computational substrate. It does not move authority yet. Its job is to prove
that the engine can hold and update a computation-facing shadow model without
falling back to Calc-owned storage objects as the real source of truth.

Phase 1 starts from the narrowed proceed boundary recorded in
[COMPUTATIONAL_SUBSTRATE_PHASE0_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_DECISION_RECORD.md).

## Phase 1 Goal

Introduce an engine-native computational storage shadow for the safe observed
subset established in Phase 0.

That means the engine must be able to represent, rebuild, and compare:

- computation-facing table, column, and cell state
- formula-cell descriptors needed for dependency and recalc work
- formula-group metadata needed by the current live queue and listener surface
- broadcaster and listener anchor identities in normalized form

The engine shadow must remain non-authoritative in this phase.

## Entry Boundary

Phase 1 should begin only from the subset that Phase 0 proved observable with
stable automated capture:

- formula-tree membership and order
- formula-track membership and order
- normalized broadcaster-state shape
- listener registration shape derived from broadcaster state
- dependency and recalc-plan correspondence on the current safe mutation set

The initial supported mutation set for the shadow should therefore be:

- `SetValue`
- formula text edits and formula insertions
- `ClearCell`
- named-range changes already covered by dependency-shadow lanes

Structural edits may be widened inside Phase 1 only after the core shadow model
is stable and pointer-free.

## Non-Goals

Phase 1 should not attempt to:

- make the engine shadow authoritative
- migrate BASM slot layout or broadcaster containers verbatim
- move `ScTokenArray` into the engine
- use raw `ScDocument`, `ScTable`, `ScColumn`, `ScFormulaCell`, or
  `ColumnBlockPositionSet` pointers as the shadow's real identity model
- broaden immediately to copy/move, clipboard rebuild, or load-time
  `CalcAfterLoad` behavior

## Required Deliverables

Phase 1 is complete only when all of the following exist:

1. an explicit engine-owned shadow schema for the narrowed subset
2. a stable builder that reconstructs the shadow from live Calc state
3. a stable mutation-update path for the Phase 1 safe mutation set
4. differential comparison helpers between the shadow and live Calc captures
5. automated validation proving the shadow survives representative mutations
6. a checked-in proceed/stop decision record for Phase 2

## Workstreams

### 1.1 Define The Engine-Owned Shadow Schema

Introduce the data model for the narrowed computational shadow.

The schema should represent:

- shadow workbook, table, column, and cell containers
- formula-cell records with stable engine-owned identifiers
- formula-group descriptors
- normalized broadcaster and listener anchor references
- links from shadow formulas to current dependency and recalc-planning inputs

Recommended constraints:

- use value-semantic identifiers such as sheet/column/row-based keys and
  explicit group ids
- avoid storing Calc object pointers as durable shadow identity
- prefer normalized engine types already used in workbook-facade,
  dependency-snapshot, and observation helpers

Likely new code homes:

- `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- `spreadsheet_engine/source/core/substrate/`

Required artifact:

- one checked-in schema note or API reference that names the Phase 1 shadow
  types and their ownership rules

### 1.2 Build The Initial Shadow Snapshot

Implement a builder that can reconstruct the entire narrowed shadow model from
live Calc state.

The builder should consume:

- the Calc-backed workbook facade
- Phase 0 computational-substrate observation helpers
- existing dependency snapshot and formula-group discovery surfaces where
  appropriate

The builder should populate:

- shadow tables, columns, and cells for the computation-facing subset
- formula metadata and group descriptors
- normalized broadcaster/listener anchor state needed for later graph work
- queue-facing state needed for Phase 1 comparison

Required artifact:

- one checked-in builder API plus at least one unit or integration lane that
  exercises full snapshot construction on representative documents

### 1.3 Add Stable Identity And Mapping Rules

Define how live Calc state maps to shadow nodes without leaking Calc ownership
into the engine model.

The mapping rules should cover:

- cell address to shadow cell identity
- formula group anchor and length to shadow group identity
- normalized broadcaster addresses and ranges to shadow listener anchors
- named-range references that affect the current narrowed subset

The mapping layer may cache Calc-side addresses or ephemeral lookup aids during
rebuild, but the final shadow state must not depend on opaque Calc pointers for
correctness.

Required artifact:

- one checked-in mapping reference or appendix that records:
  - primary ids
  - allowed ephemeral Calc-side aids
  - forbidden identity shortcuts

### 1.4 Implement Safe Mutation Update Paths

Add shadow update logic for the safe Phase 1 mutation set.

The minimum supported sequence is:

- rebuild or update after `SetValue`
- rebuild or update after formula text changes
- rebuild or update after new formula insertion
- rebuild or update after `ClearCell`
- rebuild or update after named-range edits already covered by existing shadow
  lanes

This workstream may choose either of two strategies initially:

- full shadow rebuild after each mutation, if the shadow remains fully
  engine-owned and comparison-friendly
- incremental shadow updates, if they stay simpler than rebuild and do not
  reintroduce Calc-owned authority

Phase 1 does not need to optimize for performance first. It needs to prove
representation and correctness first.

Required artifact:

- one checked-in mutation application surface with deterministic post-mutation
  shadow state

### 1.5 Add Gated Structural Widening For Representative Row/Column Edits

The original Phase 1 outline expected representative row and column structural
edits. Phase 0 did not justify broad structural authority, so Phase 1 should
treat this as a gated widening step.

Recommended order:

1. single representative row insert or delete
2. single representative column insert or delete

This widening should proceed only if:

- the shadow model for the narrowed safe subset is already stable
- the implementation can express the structural effect through engine-owned
  normalized state
- no opaque Calc-owned pointer or slot-layout dependence is needed for
  correctness

If those conditions do not hold, Phase 1 may still close successfully on the
safe mutation set, with structural widening explicitly deferred into the Phase
1 decision record rather than hidden as incomplete work.

Required artifact:

- explicit classification in the closeout decision record of whether
  representative structural edits were admitted, deferred, or blocked

### 1.6 Add Differential Validation And Freeze The Phase 1 Decision Record

Phase 1 must end with automated comparison of:

- live Calc captures from Phase 0
- engine-owned shadow state built or updated after the same mutations

At minimum, validation should compare:

- shadow cell population against the Calc-backed workbook facade
- shadow formula-tree/track-facing subsets against live captures
- shadow normalized broadcaster/listener anchors against live broadcaster
  captures
- shadow formula-group descriptors against live grouped formula observations

Phase 1 closes only with a short checked-in decision record that says one of:

- proceed to Phase 2
- proceed only on a narrower subset
- stop the computational substrate program here

## Target Surfaces

The first implementation sweep for Phase 1 should expect to introduce or touch:

- new shadow substrate code under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- new implementation files under
  `spreadsheet_engine/source/core/`
- existing facade and observation surfaces:
  - [WorkbookFacade.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx)
  - [ComputationalSubstrateObservation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObservation.hxx)
  - [DependencySnapshot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/dependency/DependencySnapshot.hxx)
  - [RecalcPlanner.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/dependency/RecalcPlanner.hxx)
- Calc-side validation lanes under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
- new standalone tests under:
  - `spreadsheet_engine/tests/unit/`

## Recommended Execution Order

Phase 1 should run in this order:

1. define shadow schema and identity rules
2. build full snapshot reconstruction from live Calc state
3. add comparison helpers between shadow and Phase 0 captures
4. support safe mutation rebuild or update on scalar, formula, clear, and
   named-range scenarios
5. only then evaluate representative row/column widening
6. freeze the Phase 1 decision record

This ordering matters because it prevents structural widening pressure from
distorting the core shadow model before the narrowed safe subset is proven.

## Validation Contract

Phase 1 should explicitly require:

- `git diff --check`
- `make -j1 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_workbook_facade`
- new standalone shadow-model tests under `spreadsheet_engine/tests/unit/`
- differential tests that compare shadow state against live Calc captures on:
  - `SetValue`
  - formula text edit or insertion
  - `ClearCell`
  - named-range edit
- if structural widening is admitted, differential tests for at least one row
  and one column representative edit
- one-shot
  `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

Where new compat or observation helpers cross the existing engine boundary, the
affected standalone or Calc lanes should also be rerun.

## Exit Criteria

Phase 1 is successful only if all of the following are true:

1. the engine has an explicit shadow schema for the narrowed subset
2. the shadow can be reconstructed from live Calc state without hidden Calc
   ownership
3. the shadow remains coherent across the safe mutation set
4. differential comparisons between shadow state and live captures are
   automated and green
5. any admitted structural widening is explicitly validated
6. the project can make a justified proceed, narrow-proceed, or stop decision
   for Phase 2

If the shadow only works by retaining opaque Calc-owned pointers as its real
identity model, then the correct outcome is to stop or narrow the program at
Phase 1 rather than pushing forward with an illusory engine-owned substrate.
