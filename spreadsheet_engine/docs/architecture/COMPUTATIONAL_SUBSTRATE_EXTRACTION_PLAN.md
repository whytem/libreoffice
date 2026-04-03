# Computational Substrate Extraction Plan

Status: active staged architecture plan

## Purpose

This document describes the architecture plan for a much larger second-stage
expansion of `spreadsheet_engine/` than the project has pursued so far.

It is based on a changed premise:

- `spreadsheet_engine/` would not only own spreadsheet semantics, dependency
  planning, and selected execution paths
- it would also become the intended owner of the live computational substrate
  that currently sits in Calc:
  - formula tree
  - broadcast track
  - broadcast-area machine
  - listener and broadcaster wiring
  - computational table and column storage, if needed
  - the execution-facing token/container model that currently centers on
    `ScTokenArray`

This is not a small follow-on cleanup stream. It is a distinct architecture
program.

## Why This Is A Different Kind Of Work

The current project boundary keeps Calc authoritative for:

- `ScDocument`, `ScTable`, and `ScColumn` storage
- formula-cell lifecycle and host-side side effects
- listener and broadcaster wiring in
  [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx)
- column broadcaster storage in
  [mtvelements.hxx](/home/ubuntu/repos/libreoffice/sc/inc/mtvelements.hxx)
- listener contexts in
  [listenercontext.hxx](/home/ubuntu/repos/libreoffice/sc/inc/listenercontext.hxx)
- `ScTokenArray` construction and reference-adjustment behavior in
  [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx)

The current engine already owns the declarative dependency and recalc-planning
side of this boundary, as shown in
[RecalcPlanner.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/dependency/RecalcPlanner.hxx).

This plan is about crossing that remaining line intentionally and replacing the
Calc-owned computational substrate with an engine-owned one.

## Target End State

The desired end state is:

- `spreadsheet_engine/` owns an engine-native computational workbook model for
  tables, columns, cells, formula groups, and dependency edges
- `spreadsheet_engine/` owns the live dependency graph, listener registration
  policy, broadcaster area indexing, and recalc queue semantics
- `spreadsheet_engine/` owns an execution-facing IR or token model for the
  engine-owned substrate rather than depending on `ScTokenArray` as the
  authoritative long-term representation
- Calc acts as the host for:
  - user-facing document storage and shell services
  - UNO, import/export, rendering, and persistence
  - document-service integrations that remain inherently host-bound
  - adapters that synchronize host state to and from the engine-owned
    computational substrate

This is the "better target" because it avoids trying to make the engine
authoritative over live graph semantics while those semantics are still
physically embedded in Calc-owned storage structures.

## Non-Goals

This plan does not attempt to:

- move UI, UNO, rendering, or persistence into `spreadsheet_engine/`
- move printer, path, or similar environment services into the engine
- replace all Calc document storage with engine storage on day one
- perform a wholesale rewrite of `ScInterpreter` as a first step
- drop the standing zero-fallback replay baseline while the migration is in
  progress

## Definition Of Success

This architecture program is successful only if all of the following become
true:

1. the engine can model the live computational state of a workbook without
   depending on Calc listener or broadcaster ownership
2. the engine can build and maintain the live dependency graph authoritatively
3. the engine can drive recalculation order from that live graph
4. the engine can do so through an engine-owned execution-facing IR or token
   model rather than through Calc-owned `ScTokenArray` as the final authority
5. Calc can remain the application host without retaining hidden computational
   ownership
6. replay and differential validation stay green throughout

## Recommended Program Structure

The program should run in deliberately gated phases.

The core rule is: no later authority shift happens until the prior shadow lane
is exact enough to justify it.

## Phase 0: Scope Freeze And Observability Foundation

Status: complete on a narrowed proceed subset

### Goal

Establish a precise, testable map of the live computational substrate that
would need to move.

### Deliverables

- explicit ownership map for:
  - formula tree
  - broadcast track
  - broadcast-area machine
  - listener contexts
  - broadcaster storage blocks
  - `ScTokenArray` creation and mutation sites
  - reference-adjustment and token-container update paths
- stable instrumentation for:
  - live dependency graph shape
  - broadcaster/listener state
  - formula-tree membership and recalc queue behavior
- a clear split between:
  - purely computational state
  - host-only document or service state

### Validation

- dedicated shadow diagnostics for broadcaster state via
  [GetBroadcasterState()](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx#L2481)
- representative edit/mutation traces for value edits, formula edits, clears,
  structural edits, and named-range changes
- standing replay lane remains green

### Risk Gate

Proceed only if the project can observe and compare the live computational
state precisely enough to support exact shadowing.

Stop if the substrate cannot be made observable without invasive behavior
changes first.

The detailed execution-ready work for this phase now lives in:

- [COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE0_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_DECISION_RECORD.md)

Phase 0 should not be treated as a soft preamble. It is the observability and
scope-freeze gate for the whole program, and later phases should not begin
until its exit criteria are met.

That gate is now met for a narrowed subset:

- formula-tree and formula-track observation
- normalized broadcaster and listener shape
- dependency and recalc correspondence on the current safe mutation set

Phase 1 may proceed, but it should start from that narrowed subset rather than
assuming broad structural or load-time authority immediately.

## Phase 1: Engine-Owned Computational Storage Shadow

The detailed execution-ready work for this phase now lives in:

- [COMPUTATIONAL_SUBSTRATE_PHASE1_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE1_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE1_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE1_DECISION_RECORD.md)

Status: complete on the narrowed Phase 0 subset, widened to representative
row-insert and column-delete rebuild coverage

### Goal

Introduce an engine-native computational storage model that can shadow the
live Calc computational state without yet becoming authoritative.

### Scope

- engine-native table, column, and cell state for computation-facing data
- formula-group metadata
- broadcaster/listener anchor identifiers
- mapping between Calc addresses/objects and engine-owned computational nodes

### Requirements

- the engine shadow must be buildable from live Calc state
- the shadow must survive ordinary mutation sequences
- the shadow must not require Calc to relinquish storage authority yet

### Validation

- differential tests that compare shadow table/column/cell population against
  Calc after representative edits
- mutation replay for:
  - `SetValue`
  - `SetString`
  - `SetFormula`
  - `ClearCell`
  - row/column insert/delete

### Risk Gate

Proceed only if the engine shadow can represent the needed computational state
without carrying opaque Calc-only pointers as its real source of truth.

Stop if the shadow collapses into a thin mirror that still fundamentally
depends on Calc storage ownership for correctness.

## Phase 2: Live Dependency Graph Shadow

The detailed execution-ready work for this phase now lives in:

- [COMPUTATIONAL_SUBSTRATE_PHASE2_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE2_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE2_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE2_DECISION_RECORD.md)

Status: complete on the narrowed Phase 1 subset, with exact-or-normalized
graph matches and representative delayed/structural coverage

### Goal

Build an engine-owned live dependency graph on top of the shadow substrate and
compare it continuously to Calc's live listener and broadcaster state.

### Scope

- formula-cell to dependency-edge registration
- area broadcaster indexing
- listener start/end policy
- group-aware dependency and listener behavior

### Requirements

- exact or explainably equivalent shadow graph state after representative edits
- explicit handling for delayed listener startup and delayed broadcaster
  deletion currently visible in Calc
- no hidden dependency on Calc-owned broadcaster containers as the real source
  of graph truth

### Validation

- graph-diff tests after representative mutations
- broadcaster-state comparisons after edit, clear, insert/delete, copy/move,
  and load scenarios
- existing dependency and recalc shadow lanes remain green

### Risk Gate

Proceed only if the shadow graph matches Calc closely enough to trust as a
future authority candidate.

Recommended threshold:

- exact or intentionally-normalized shadow matches on the safe mutation set
- no persistent unexplained under- or over-registration of listeners

Stop if correctness requires too much Calc-owned special casing.

Phase 3 may proceed, but only from the narrowed graph subset admitted by the
Phase 2 decision record rather than by assuming broad graph or storage
authority.

## Phase 3: Engine-Owned Execution IR Boundary

The detailed execution-ready work for this phase now lives in:

- [COMPUTATIONAL_SUBSTRATE_PHASE3_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE3_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE3_IR_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE3_IR_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE3_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE3_DECISION_RECORD.md)

Status: complete on the narrowed Phase 2 subset, with deterministic lowering,
reference-shape coherence, representative structural-update coverage, and
exact-or-normalized IR differential validation

### Goal

Define the execution-facing representation that the engine will own instead of
continuing to rely on `ScTokenArray` as the ultimate computational authority.

### Scope

- engine-native execution IR or token model for the migrated substrate
- importer/lowering path from current Calc compilation output
- support for range/union/intersection/reference-shape behavior needed by the
  live graph and execution surface
- explicit boundary for what remains host-specific

### Requirements

- preserve compile and replay behavior for the in-scope formulas
- support reference adjustment and structural-update semantics needed by the
  migrated computational substrate
- avoid creating a permanent dual-authority model where both `ScTokenArray`
  and the engine IR are equally authoritative

### Validation

- compiler round-trip tests
- targeted comparison of reference adjustment after shift/move/delete
- replay and evaluator lanes remain green

### Risk Gate

Proceed only if the engine IR can cover the migrated substrate without simply
embedding `ScTokenArray` semantics wholesale forever.

Stop if the only viable path is to keep Calc token containers as the real
authority under a thin wrapper.

Phase 4 may proceed, but only from the admitted IR-shadow subset recorded in
the Phase 3 decision record rather than by assuming broad execution or
structural authority immediately.

## Phase 4: Engine-Authoritative Dependency And Recalc Pilot

The detailed execution-ready work for this phase now lives in:

- [COMPUTATIONAL_SUBSTRATE_PHASE4_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE4_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE4_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE4_DECISION_RECORD.md)

Status: complete on the narrowed graph-and-queue-authoritative subset, with
deterministic rollback/rejection and execution-IR comparison retained as
observation data rather than a hard rollback gate

### Goal

Make the engine authoritative for live dependency graph updates and recalc
queue construction on a safe pilot surface while Calc still hosts the document.

### Scope

- safe mutation subset first:
  - `SetValue`
  - `SetString`
  - `SetFormula`
  - `ClearCell`
- engine-authoritative dependency graph maintenance
- engine-authoritative recalc queue derivation
- Calc verification/rollback path

### Requirements

- exact graph updates for the pilot surface
- exact or intentionally equivalent recalc queue behavior
- robust rollback when shadow and live state diverge

### Validation

- current dependency shadow and recalc-authority lanes
- targeted Cppunit coverage for pilot mutations
- one-shot replay remains zero fallback

### Risk Gate

Proceed only if the pilot can run authoritatively without persistent rollback
or under-scheduling.

Stop if the engine graph is still too dependent on Calc-owned repair logic.

## Phase 5: Engine-Authoritative Formula Lifecycle Pilot

The detailed execution-ready work for this phase now lives in:

- [COMPUTATIONAL_SUBSTRATE_PHASE5_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE5_PLAN.md)

Status: next frontier, but only on the narrower admitted Phase 4 subset

### Goal

Move the computational formula lifecycle for the pilot subset onto the engine
substrate instead of just letting the engine shadow or verify it.

### Scope

- formula insertion/update/removal for the pilot surface
- formula-tree membership and grouping semantics
- listener registration and teardown for pilot formulas
- synchronization back to Calc host-visible document state

### Requirements

- no hidden reintroduction of Calc-owned listener or formula-tree authority
- stable behavior across load, undo-like mutations, and grouped formulas in
  the pilot surface

### Validation

- targeted lifecycle and group-split tests
- differential formula-tree and broadcaster-state checks
- replay and evaluator lanes remain green

### Risk Gate

Proceed only if the engine can own lifecycle for the pilot surface without
Calc silently repairing the state afterward.

Stop if formula lifecycle remains inseparable from host-only document
ownership.

## Phase 6: Structural Edit And Reference-Update Expansion

### Goal

Expand from the safe pilot surface to the reference-update and structural-edit
paths that currently depend heavily on `ScTokenArray` and Calc storage
operations.

### Scope

- row/column insert/delete
- tab moves or deletions as appropriate
- range/union/intersection token and IR updates
- named-range and shared-formula interaction with structural edits

### Requirements

- engine-owned reference-update logic for the migrated substrate
- clear policy for retained host-only paths
- no silent fallback to Calc as hidden authority

### Validation

- structural edit differential tests
- reference-adjustment regression tests
- existing workbook-facade, dependency, and replay lanes remain green

### Risk Gate

Proceed only if structural edits can be handled by the engine-owned substrate
without unacceptable complexity or semantic drift.

Stop if the token/reference update surface proves too Calc-specific to migrate
cleanly.

## Phase 7: Host Boundary Re-Cut And Rollout Decision

### Goal

Decide whether the migrated computational substrate is now good enough to
become the intended long-term architecture, and if so, define the new stable
Calc/engine boundary.

### Scope

- final ownership map
- retained host-only services list
- rollout plan for remaining workbook classes and mutation classes
- rollback/deactivation strategy

### Requirements

- the new boundary must be simpler, not more tangled
- the engine must clearly own computational state and graph authority
- Calc must clearly own document hosting and user-facing services

### Validation

- full standing validation contract
- memory and performance comparisons against current baseline
- representative load/edit/recalc/end-to-end workbook scenarios

### Risk Gate

Do not roll forward into broad adoption unless all of the following are true:

- correctness is stable
- memory overhead is acceptable
- recalc performance is acceptable
- the new boundary is operationally understandable

If those conditions are not met, the program should close as an exploratory
architecture effort rather than force adoption.

## Standing Validation Contract

Every phase should keep the following green where relevant:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Additional validation that this program specifically needs:

- broadcaster-state differential checks
- live dependency-graph differential checks
- formula-tree membership and queue-order comparisons
- structural edit and reference-adjustment regression lanes
- memory and recalc-performance tracking on representative workloads

## Recommended Decision Framework

This program should be treated as viable only if each phase earns the next.

The default posture should be:

- continue if a phase produces a cleaner engine-owned authority boundary
- pause if a phase is technically possible but introduces a more tangled
  cross-layer model
- stop if correctness depends on Calc remaining the de facto hidden authority

That discipline matters because the attractive end state is real, but so is
the risk of creating a dual-authority system that is worse than the current
boundary.

## Bottom-Line Recommendation

This is a plausible architecture direction if and only if the project is
willing to treat it as a major second-stage computational substrate migration,
not as a continuation of the narrower extraction streams that came before it.

The best next move, if this direction is pursued, is Phase 0:

- freeze the true migration scope
- build the observability needed for exact shadow comparisons
- refuse to commit to later authority shifts until the early shadow phases are
  exact enough to justify them

That keeps the project honest and makes it possible to stop at the right time
if the new boundary turns out not to be worth the cost.
