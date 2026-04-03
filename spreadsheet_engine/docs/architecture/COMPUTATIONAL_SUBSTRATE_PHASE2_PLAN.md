# Computational Substrate Phase 2 Plan

Status: completed implementation and closeout record for Phase 2

## Purpose

This document is the execution-ready plan for Phase 2 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

Phase 2 is the first phase that introduces an engine-owned shadow of the live
dependency graph on top of the completed Phase 1 computational substrate
shadow.

It does not move graph authority yet. Its job is to prove that the engine can
reconstruct, compare, and maintain a normalized live dependency graph shadow
without treating Calc listener/broadcaster containers as the real source of
truth.

Phase 2 begins from the proceed boundary recorded in
[COMPUTATIONAL_SUBSTRATE_PHASE1_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE1_DECISION_RECORD.md).

The checked-in closeout decision now lives in
[COMPUTATIONAL_SUBSTRATE_PHASE2_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE2_DECISION_RECORD.md).

## Outcome

Phase 2 is complete.

The project can proceed to Phase 3 only on the narrowed subset validated here:

- engine-owned graph shadow nodes for formula cells, formula groups,
  listener anchors, and cell/area broadcasters
- broadcaster-to-listener edge reconstruction
- exact-or-normalized graph comparisons on the safe mutation set
- explicit delayed listener / delayed broadcaster handling
- representative single-row insert and single-column delete rebuild coverage

Phase 2 did not make the graph authoritative, and it did not widen into
copy/move, clipboard, load-time, BASM layout, or Calc container migration.

## Phase 2 Goal

Introduce an engine-native live dependency graph shadow for the admitted Phase
1 subset.

That means the engine must be able to represent, rebuild, and compare:

- listener-anchor registration for formula cells and formula groups
- broadcaster-to-listener edges for cell and area broadcasters
- group-aware listener anchoring and normalized membership
- graph-facing correspondence with the current dependency and recalc-planning
  inputs

The Phase 2 graph shadow must remain non-authoritative in this phase.

## Entry Boundary

Phase 2 should start only from the subset that Phase 1 now shadows and
validates cleanly:

- non-empty computation-facing cell population
- formula descriptors and formula-group descriptors
- formula-tree and formula-track membership as observed state
- normalized broadcaster and listener anchor state
- rebuild-based shadow maintenance for:
  - `SetValue`
  - formula edit or insertion
  - `ClearCell`
  - named-range edit
- representative structural rebuilds for:
  - single row insert
  - single column delete

The initial graph-supported mutation set should therefore be:

- `SetValue`
- formula text edits and formula insertions
- `ClearCell`
- named-range edits already covered by dependency-shadow lanes
- one representative row insert
- one representative column delete

Delayed listener startup and delayed broadcaster deletion are admitted because
Phase 0 already proved they are observable and comparable.

## Non-Goals

Phase 2 should not attempt to:

- make the graph shadow authoritative
- move BASM slot layout into the engine
- migrate Calc broadcaster containers or listener contexts verbatim
- move `ScTokenArray` into the engine
- use `ScDocument`, `ScFormulaCell`, `SvtBroadcaster`, broadcaster stores, or
  BASM slot objects as durable graph identity
- broaden immediately to full copy/move, clipboard rebuild, or load-time
  `CalcAfterLoad` authority

## Required Deliverables

Phase 2 is complete only when all of the following exist:

1. an explicit engine-owned graph-shadow schema for the admitted subset
2. a stable builder that reconstructs the graph shadow from the Phase 1
   shadow plus normalized live observation
3. a stable graph-diff surface that classifies exact, normalized-equivalent,
   and mismatched graph states
4. automated validation for the safe mutation set and admitted structural
   cases
5. explicit handling and validation for delayed listener startup and delayed
   broadcaster deletion
6. a checked-in proceed/stop decision record for Phase 3

## Workstreams

### 2.1 Define The Engine-Owned Graph Shadow Schema

Introduce the data model for the narrowed live dependency graph shadow.

The schema should represent:

- graph nodes for formula cells and formula groups
- normalized listener-anchor nodes
- cell and area broadcaster nodes
- directed broadcaster-to-listener edges
- graph metadata needed for later Phase 4 authority work

Recommended constraints:

- use value-semantic ids derived from the Phase 1 shadow and normalized
  observation state
- avoid storing Calc listener or broadcaster pointers as durable graph
  identity
- keep the graph schema explicitly separate from BASM slot layout

Likely new code homes:

- `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- `spreadsheet_engine/source/core/`

Required artifact:

- one checked-in schema or API note that names the Phase 2 graph types and
  their ownership rules

### 2.2 Build Full Graph Reconstruction From Shadow Plus Observation

Implement a builder that reconstructs the entire admitted graph shadow from:

- the completed Phase 1 computational shadow
- normalized broadcaster and listener observation capture
- existing dependency snapshot and recalc-planning surfaces where appropriate

The builder should populate:

- graph nodes for formula cells, formula groups, cell broadcasters, and area
  broadcasters
- normalized graph edges from broadcasters to listener anchors
- graph-facing correspondence to formula-tree and formula-track membership
- graph metadata needed for later authority comparison

Required artifact:

- one checked-in builder API plus at least one standalone or Calc-backed lane
  that exercises full graph reconstruction on representative documents

### 2.3 Add Stable Graph Identity And Equivalence Rules

Define how live Calc graph state maps to the engine graph shadow without
leaking Calc ownership into the graph model.

The mapping rules should cover:

- formula cell to graph node identity
- formula group anchor and length to group node identity
- cell broadcaster address to broadcaster node identity
- area broadcaster range to broadcaster node identity
- listener snapshots to normalized listener-anchor identity
- intentionally-normalized equivalence for group listeners, delayed startup,
  and delayed deletion cases where exact container behavior is not the real
  desired contract

Required artifact:

- one checked-in mapping and equivalence reference that records:
  - primary ids
  - normalized-equivalent cases
  - allowed ephemeral Calc-side aids
  - forbidden identity shortcuts

### 2.4 Add Graph Diff And Safe-Mutation Differential Lanes

Implement graph comparison helpers and wire them into automated tests for the
admitted safe mutation set.

The minimum supported sequence is:

- graph rebuild and comparison after `SetValue`
- graph rebuild and comparison after formula text changes
- graph rebuild and comparison after new formula insertion
- graph rebuild and comparison after `ClearCell`
- graph rebuild and comparison after named-range edits already covered by
  existing dependency-shadow lanes

Phase 2 may choose rebuild-only graph updates initially, as long as the graph
state remains fully engine-owned and differential-friendly.

Required artifact:

- one checked-in graph comparison surface with explicit verdict categories and
  deterministic post-mutation graph state

### 2.5 Add Explicit Delayed-Listener And Structural Gate Coverage

Phase 2 must prove that the graph shadow can survive the two live listener
edge cases already admitted by observability, plus the representative
structural widening carried forward from Phase 1.

This workstream should add graph-shadow differential validation for:

1. delayed listener startup
2. delayed broadcaster deletion
3. representative row insert
4. representative column delete

If copy/move or load-time `CalcAfterLoad` behavior is explored here, it should
be classified explicitly as admitted, deferred, or blocked rather than left
implicit.

Required artifact:

- explicit classification in the closeout decision record of which special
  cases and structural cases are admitted, deferred, or blocked

### 2.6 Freeze Differential Closeout And The Phase 2 Decision Record

Phase 2 must end with automated comparison between:

- live Calc graph captures
- engine-owned graph shadow state built from the same workbook state

At minimum, validation should compare:

- broadcaster node population
- listener-anchor node population
- broadcaster-to-listener edge sets
- formula-group listener anchoring
- formula-tree and formula-track-facing graph subsets

Phase 2 closes only with a checked-in decision record that says one of:

- proceed to Phase 3
- proceed only on a narrower graph subset
- stop the computational substrate program here

## Target Surfaces

The first implementation sweep for Phase 2 should expect to introduce or
touch:

- new graph-shadow code under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- new implementation files under
  `spreadsheet_engine/source/core/`
- completed Phase 1 surfaces:
  - [ComputationalShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadow.hxx)
  - [ComputationalShadowBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx)
  - [ComputationalShadowComparison.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx)
- observation and compat surfaces:
  - [ComputationalSubstrateObservation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObservation.hxx)
  - [RecalcShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcShadow.hxx)
  - [DependencyShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/DependencyShadow.hxx)
- Calc-side validation lanes under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
- standalone graph-shadow tests under:
  - `spreadsheet_engine/tests/unit/`

## Recommended Execution Order

Phase 2 should run in this order:

1. define graph schema and equivalence rules
2. build full graph reconstruction from Phase 1 shadow plus observation
3. add graph diff helpers and exact/normalized verdicts
4. support safe-mutation rebuild and comparison on the admitted subset
5. validate delayed-listener and representative structural cases
6. freeze the Phase 2 decision record

This ordering matters because it keeps the graph contract explicit before the
special-case and widening pressure arrives.

## Validation Contract

Phase 2 should explicitly require:

- `git diff --check`
- targeted computational-graph cases in `CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_workbook_facade`
- new standalone graph-shadow tests under `spreadsheet_engine/tests/unit/`
- differential tests that compare graph shadow state against live Calc capture
  on:
  - `SetValue`
  - formula text edit or insertion
  - `ClearCell`
  - named-range edit
  - delayed listener startup
  - delayed broadcaster deletion
  - representative row insert
  - representative column delete
- if copy/move or load-time cases are explored, explicit verdict coverage for
  whether they are admitted or deferred
- existing dependency and recalc shadow lanes that overlap the touched compat
  surfaces
- one-shot
  `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

Where new compat or observation helpers cross the existing engine boundary,
the affected standalone or Calc lanes should also be rerun.

## Exit Criteria

Phase 2 is successful only if all of the following are true:

1. the engine has an explicit graph-shadow schema for the admitted subset
2. the graph shadow can be reconstructed from Phase 1 shadow plus live
   observation without hidden Calc-owned graph identity
3. the graph shadow remains coherent across the admitted mutation set
4. exact or intentionally-normalized differential comparisons are automated
   and green
5. delayed-listener and representative structural cases are explicitly
   classified and validated
6. the project can make a justified proceed, narrow-proceed, or stop decision
   for Phase 3

If the graph shadow only works by treating Calc listener/broadcaster
containers as the real identity model, then the correct outcome is to stop or
narrow the program at Phase 2 rather than pushing forward with a false
engine-owned graph.
