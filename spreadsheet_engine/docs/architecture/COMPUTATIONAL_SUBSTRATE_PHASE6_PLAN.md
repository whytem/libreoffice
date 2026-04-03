# Computational Substrate Phase 6 Plan

Status: active execution-ready plan for Phase 6

## Purpose

This document is the execution-ready plan for Phase 6 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

Phase 6 begins from the narrowed proceed boundary recorded in
[COMPUTATIONAL_SUBSTRATE_PHASE5_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE5_DECISION_RECORD.md).

Its job is to widen the computational substrate program from scalar
formula-lifecycle authority into the first structural-edit and
reference-update surface that still sits heavily inside Calc storage and token
plumbing.

Phase 6 is the first phase that asks whether the engine can own a mutation
whose semantics are defined not only by formula insertion or removal, but also
by reference shifting, formula relocation, and structural invalidation.

## Phase 6 Goal

Make `spreadsheet_engine/` authoritative for a narrowly admitted structural
and reference-update subset on top of the Phase 5 lifecycle-authoritative
pilot surface.

That means the engine must be able to:

- accept a tightly bounded set of structural mutations
- derive the post-mutation computational, graph, queue, and reference-update
  state from engine-owned substrate logic rather than from Calc-owned
  `ScTokenArray`-driven repair
- package the resulting host synchronization work for Calc
- verify that Calc does not silently widen, repair, or replace the admitted
  structural answer
- reject, roll back, or defer anything outside the admitted structural subset

Phase 6 is successful only if the engine becomes the real source of admitted
structural and reference-update behavior, not just an after-the-fact observer
of Calc storage repair.

## Entry Boundary

Phase 6 should start only from the narrower Phase 5 authority subset that was
actually proven:

- scalar formula insertion
- scalar formula replacement that preserves single-cell shape
- scalar formula removal through `ClearCell`
- clean-baseline entry only
- exact queue verification
- exact computational verification
- exact graph verification
- execution-IR comparison retained as observation data

Phase 6 should begin narrower than the full architectural Phase 6 scope.

The initial structural and reference-update pilot should be:

- representative row insert on a workbook slice that only contains ordinary
  scalar formulas
- representative column delete on a workbook slice that only contains
  ordinary scalar formulas
- local reference-shape updates that do not require shared-group lifecycle
  repair
- no external-reference, copy/move, clipboard, load-time, or undo-like
  behavior

The initial pilot should explicitly exclude:

- shared-group creation, split, merge, or repair
- named-range structural authority in the live path
- sheet insert/delete/move authority
- range/union/intersection widening beyond what is directly required by the
  admitted row-insert and column-delete cases
- any mutation whose correctness still depends on retained Calc listener or
  broadcaster ownership in ways the engine cannot model directly

Representative wider cases may still appear in validation lanes, but they
should not be admitted into live authority until the narrower structural path
is proven.

## Non-Goals

Phase 6 should not attempt to:

- move full structural-edit document authority out of Calc
- move `ScTokenArray` ownership into the engine in one step
- widen immediately to sheet insert/delete/move authority
- widen immediately to named-range structural authority
- widen immediately to shared-group structural repair
- turn execution-IR comparison into a hard rollback gate unless the evidence
  clearly supports it
- move copy, move, clipboard, load-time, or undo-like structural behavior into
  the first admitted authority slice
- move retained host-only document or environment services

## Required Deliverables

Phase 6 is complete only when all of the following exist:

1. an explicit Phase 6 structural and reference-update authority contract
   naming the admitted structural mutation subset and defer list
2. an engine-owned state and sync model for admitted structural-reference
   transitions
3. a stable engine-authored transition path for the admitted row-insert and
   column-delete subset, including reference-update behavior
4. a Calc bridge that applies the structural answer, synchronizes host-visible
   state, and detects silent post-apply repair
5. automated validation for applied, normalized, rejected, rolled-back, and
   repair-detected structural cases on the admitted subset
6. a checked-in proceed or stop decision record for Phase 7

## Workstreams

### 6.1 Freeze The Structural Authority Contract And Admitted Mutation Surface

Freeze the exact Phase 6 authority boundary before the implementation widens.

The contract should name:

- the admitted structural mutation types
- the required clean-baseline and pre-shape conditions
- the exact admitted reference-update behaviors
- which structural mutation classes remain validation-only, deferred, or
  blocked
- whether any normalized-equivalent outcomes are accepted for graph, queue,
  computational, or IR-facing state

The contract should also record what counts as "engine-owned structural
authority" in this phase so later work cannot silently treat Calc repair as
acceptable.

Required artifact:

- one checked-in Phase 6 structural authority contract note
- one checked-in admitted/deferred structural mutation matrix or equivalent
  classification

### 6.2 Define The Engine-Owned Structural And Reference-Update State Model

Define the structural-facing state model the engine will use for the pilot.

The model should make explicit:

- the pre-mutation computational, graph, queue, and IR inputs required for
  admitted structural transitions
- the engine-owned representation for admitted reference-update outcomes
- the host synchronization work Calc must apply after an engine-authored
  structural answer
- the verification state Calc must compare after synchronization

Recommended constraints:

- use engine-owned ids and value-semantic records
- keep Calc token-container identity, listener containers, and broadcaster
  storage out of durable structural authority state
- model host synchronization explicitly instead of relying on hidden document
  repair after mutation

Required artifact:

- one checked-in schema or API note for the Phase 6 structural and
  reference-update state model

### 6.3 Build The Engine-Authored Structural Transition Path

Implement the engine-owned transition path for the admitted structural subset.

The first implementation may still be rebuild-backed, but it must make the
engine the real source of:

- admitted row-insert and column-delete structural transitions
- admitted reference updates for scalar formulas on that subset
- post-mutation computational, graph, and recalc state on the admitted subset
- explicit unsupported or rejected verdicts when a mutation drifts outside the
  admitted structural surface

Required artifact:

- one checked-in structural transition API plus at least one standalone or
  Calc-backed lane that exercises the admitted row-insert and column-delete
  cases

### 6.4 Wire The Calc Structural Sync And Repair-Detection Bridge

Add the Calc-side bridge that consumes the Phase 6 structural answer.

The bridge should:

- package the live mutation into the admitted structural pilot input
- apply the engine-derived structural synchronization work to Calc-hosted
  state
- verify the resulting live queue, computational, and graph state
- detect whether Calc has silently repaired or widened the structural answer
- restore the pre-apply state on divergence

The bridge should remain opt-in and narrow in this phase.

Required artifact:

- one checked-in compat surface for authoritative structural apply, verify,
  silent-repair detection, and rollback on the admitted subset

### 6.5 Add Structural Differential, Rejection, And Rollback Lanes

Phase 6 must prove that admitted structural mutations work and that rejected
or rolled-back cases are deliberate.

Validation should cover:

- exact admitted structural application for the selected row-insert and
  column-delete pilot cases
- any explicitly accepted normalized-equivalent structural cases
- rollback when structural verification fails
- rejection when the baseline is not clean
- rejection when the mutation class is deferred or blocked
- detection of silent post-apply structural repair if it appears

This workstream should keep the standing computational, graph, IR,
workbook-facade, and replay baselines green while adding the new structural
authority lane.

Required artifact:

- one checked-in differential surface with explicit verdict categories for
  applied, normalized-equivalent, rolled back, rejected, and repair-detected
  structural cases

### 6.6 Freeze Phase 6 Closeout And The Proceed Decision

Phase 6 closes only with a checked-in decision record that says one of:

- proceed to Phase 7 on the admitted structural-authoritative subset
- proceed only on a narrower structural subset
- stop the computational substrate program here

The closeout must explicitly classify:

- what is now truly engine-authoritative in structural and reference-update
  behavior
- what still depends on Calc as host, verifier, or structural owner
- whether named-range or shared-group structural behavior remains deferred or
  can partially proceed
- whether execution-IR comparison should remain observational or become a
  harder gate in later phases
- which divergence or repair patterns remain unacceptable

Required artifact:

- one checked-in Phase 6 decision record with explicit proceed or stop
  reasoning

## Target Surfaces

The first implementation sweep for Phase 6 should expect to introduce or
touch:

- new or extended structural and reference-update code under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- completed substrate surfaces:
  - [ComputationalShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadow.hxx)
  - [DependencyGraphShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx)
  - [ExecutionIr.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIr.hxx)
  - [LifecyclePilot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilot.hxx)
  - [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- Calc compat layers under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- likely new or extended Calc pilot bridges adjacent to:
  - [ComputationalSubstrateLifecycle.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx)
  - [ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)
  - [RecalcQueueExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx)
- Calc validation lanes under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_compile_diff.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_compile_diff.cxx)
- standalone tests under `spreadsheet_engine/tests/unit/`

## Recommended Execution Order

Phase 6 should run in this order:

1. freeze the structural authority contract and admitted mutation surface
2. define the engine-owned structural and reference-update state model
3. build the engine-authored structural transition path
4. wire the Calc structural sync and repair-detection bridge
5. add structural differential, rejection, and rollback lanes
6. freeze the Phase 6 decision record

## Validation Contract

Phase 6 validation should keep the existing substrate and replay baselines
green while adding explicit structural-authority coverage.

The minimum closeout contract should be:

- targeted structural-authority cases in `CppunitTest_sc_ucalc_dependency_shadow`
- targeted workbook-facade and compile-diff cases where the structural pilot
  depends on admitted reference-update or formula metadata
- a standalone structural or computational-substrate lane under
  `spreadsheet_engine/build_check`
- existing:
  - `spreadsheetengine_computational_graph_tests`
  - `spreadsheetengine_computational_ir_tests`
  - `spreadsheetengine_computational_substrate_tests`
  - `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Where Phase 6 introduces runtime toggles, validation should exercise both:

- accepted authoritative structural application on the admitted subset
- explicit rollback, rejection, or repair-detected outcomes on out-of-contract
  or divergent cases

## Exit Criteria

Phase 6 should be considered complete only if all of the following are true:

1. the engine is the real source of admitted structural and reference-update
   transitions on the narrowed pilot subset
2. the admitted structural mutation set applies with exact or explicitly
   documented normalized-equivalent verification
3. rollback is deterministic, exercised, and not merely theoretical
4. deferred structural classes are rejected explicitly rather than drifting
   into accidental partial authority
5. any silent Calc-side structural repair is either absent on the admitted
   subset or detected and treated as unacceptable
6. standing replay and substrate differential lanes remain green
7. the checked-in decision record states clearly whether Phase 7 should
   proceed, narrow, or stop

Phase 6 should stop rather than proceed if either of these becomes true:

- the engine structural answer still depends on Calc-owned repair logic in
  ways the engine cannot model directly
- rollback, rejection, or repair-detected outcomes become the normal result
  even on the admitted structural subset

## Phase 6 Definition Of Success

Phase 6 is a success if it proves that a narrow engine-authored structural and
reference-update surface is practical without hiding Calc as the real
structural owner.

It is not a success if it merely lets the engine describe structural changes
that Calc then silently rewrites or repairs on its own.
