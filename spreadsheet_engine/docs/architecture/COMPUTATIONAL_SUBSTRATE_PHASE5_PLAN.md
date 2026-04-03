# Computational Substrate Phase 5 Plan

Status: active execution-ready plan for Phase 5

## Purpose

This document is the execution-ready plan for Phase 5 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

Phase 5 begins from the narrowed proceed boundary recorded in
[COMPUTATIONAL_SUBSTRATE_PHASE4_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE4_DECISION_RECORD.md).

Its job is to move the first narrow slice of computational formula lifecycle
authority onto the engine-owned substrate rather than leaving lifecycle
creation, teardown, grouping, and listener wiring as purely Calc-owned repair
logic around an otherwise engine-authored graph-and-queue answer.

Phase 5 is the first phase that asks whether the engine can own the life of a
formula-bearing cell on the admitted subset, not just shadow it or derive
post-mutation dependency state from it.

## Phase 5 Goal

Make `spreadsheet_engine/` authoritative for the computational formula
lifecycle on the admitted pilot subset.

That means the engine must be able to:

- accept a narrowed set of formula-bearing mutations
- derive the post-mutation computational, graph, and queue state from
  engine-owned lifecycle state rather than from Calc-owned formula-cell repair
- define formula insertion, update, and removal semantics for the admitted
  subset
- define formula-tree membership, formula-track membership, and admitted
  formula-group semantics for the admitted subset
- package the resulting host synchronization work for Calc
- verify that Calc is not silently re-owning or repairing the lifecycle after
  the engine answer has been applied

Phase 5 is successful only if the engine becomes the real lifecycle source for
the admitted subset instead of merely wrapping an already-repaired Calc result
in more adapter code.

## Entry Boundary

Phase 5 should start only from the narrowed Phase 4 authority subset that was
actually proven:

- engine-authored dependency-graph update for:
  - `SetValue`
  - scalar `SetString`
  - formula text edit via `SetString`
  - direct `SetFormula`
  - `ClearCell`
- engine-authored recalc-queue derivation for the same admitted set
- exact queue verification after application
- exact graph verification after application
- deterministic rejection for dirty or out-of-contract baselines
- deterministic rollback on queue-or-graph verification failure
- execution-IR comparison retained as observation data only

Phase 5 should begin narrower than the whole Phase 4 subset.

The initial lifecycle-authoritative surface should be:

- formula text edit on an existing scalar formula cell
- direct `SetFormula` insertion of a scalar formula cell
- formula replacement that preserves a single-cell formula shape
- `ClearCell` removal of a scalar formula cell

The initial lifecycle pilot should explicitly exclude:

- value-only edits that do not change formula lifecycle shape
- named-range authority in the live lifecycle path
- shared-group creation, split, merge, or repair as an admitted runtime path
- structural edits
- copy, move, clipboard, load-time, undo-like, or repair-heavy mutations

Representative group-aware cases may still appear in the validation lane, but
they should not be admitted into live authority until the narrower scalar
formula lifecycle path is proven.

## Non-Goals

Phase 5 should not attempt to:

- move full document mutation authority out of Calc
- make listener or broadcaster storage authoritative in the engine
- widen immediately to shared-group lifecycle authority
- widen immediately to structural-edit lifecycle authority
- turn execution-IR comparison into a hard rollback gate unless the evidence
  clearly supports it
- move copy, move, clipboard, load-time, or undo-like lifecycle behavior into
  the first admitted authority slice
- move external-reference cache ownership or other host-heavy services

## Required Deliverables

Phase 5 is complete only when all of the following exist:

1. an explicit Phase 5 lifecycle authority contract naming the admitted
   formula-bearing mutation subset and the exact defer list
2. an engine-owned lifecycle state model for admitted formula cells and any
   admitted group metadata
3. a stable engine-authored lifecycle transition path for formula insertion,
   update, and removal on the admitted subset
4. a Calc bridge that applies the lifecycle answer, synchronizes host-visible
   state, and detects silent post-apply repair
5. automated validation for applied, normalized, rejected, and rolled-back
   lifecycle cases on the admitted subset
6. a checked-in proceed or stop decision record for Phase 6

## Workstreams

### 5.1 Freeze The Lifecycle Authority Contract And Admitted Mutation Surface

Freeze the exact Phase 5 authority boundary before implementation widens.

The contract should name:

- the admitted lifecycle-bearing mutation types
- the required clean-baseline and pre-shape conditions
- which formula lifecycle changes are admitted:
  - insertion
  - in-place formula replacement
  - formula removal
- which lifecycle changes remain validation-only, deferred, or blocked
- whether any normalized-equivalent outcomes are accepted for formula-tree,
  formula-track, formula-group, listener, or IR-facing state

The contract should also record what counts as “engine-owned lifecycle” in
this phase so later work cannot silently treat Calc repair as acceptable.

Required artifact:

- one checked-in Phase 5 lifecycle authority contract note
- one checked-in admitted/deferred lifecycle mutation matrix or equivalent
  classification

The checked-in artifacts for this workstream are:

- [COMPUTATIONAL_SUBSTRATE_PHASE5_LIFECYCLE_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE5_LIFECYCLE_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE5_LIFECYCLE_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE5_LIFECYCLE_MATRIX.md)

### 5.2 Define The Engine-Owned Lifecycle State And Sync Model

Define the lifecycle-facing state model the engine will use for the pilot.

The model should make explicit:

- pre-mutation computational, graph, and IR inputs required for lifecycle
  transitions
- the durable engine-owned identity for admitted formula-bearing cells
- admitted formula-tree and formula-track membership outputs
- admitted formula-group metadata, if any, on the narrower subset
- the host synchronization work Calc must apply after an engine-authored
  lifecycle answer
- the verification state Calc must compare after synchronization

Recommended constraints:

- use engine-owned ids and value-semantic records
- keep Calc formula-cell pointers, listener containers, and token-container
  identity out of durable lifecycle authority state
- model host synchronization explicitly instead of relying on hidden document
  repair after mutation

Required artifact:

- one checked-in schema or API note for the Phase 5 lifecycle state and sync
  model

### 5.3 Build The Engine-Authored Lifecycle Transition Path

Implement the engine-owned lifecycle transition path for the admitted subset.

The first implementation may still be rebuild-backed, but it must make the
engine the real source of:

- formula cell creation on direct formula insertion
- formula metadata replacement on admitted formula edits
- formula removal on admitted clears
- formula-tree and formula-track membership derived from the new lifecycle
  state
- any admitted formula-group metadata carried by the narrowed subset

The transition path should also emit explicit unsupported or rejected verdicts
for any mutation that drifts outside the admitted lifecycle subset.

Required artifact:

- one checked-in lifecycle transition API plus at least one standalone or
  Calc-backed lane that exercises the admitted insertion, replacement, and
  removal cases

### 5.4 Wire The Calc Lifecycle Sync And Repair-Detection Bridge

Add the Calc-side bridge that consumes the Phase 5 lifecycle answer.

The bridge should:

- package the live mutation into the admitted lifecycle pilot input
- apply the engine-derived lifecycle synchronization work to Calc-hosted state
- verify the resulting live formula-tree, formula-track, graph-facing state,
  and admitted lifecycle shape
- detect whether Calc has silently repaired or widened the lifecycle beyond
  the engine answer
- restore the pre-apply state on divergence

The bridge should remain opt-in and narrow in this phase.

Required artifact:

- one checked-in compat surface for authoritative lifecycle apply, verify,
  silent-repair detection, and rollback on the admitted subset

### 5.5 Add Lifecycle Differential, Rejection, And Rollback Lanes

Phase 5 must prove that accepted lifecycle mutations work and that rejected or
rolled-back cases are deliberate.

Validation should cover:

- exact admitted lifecycle application for formula insertion, replacement, and
  removal
- any explicitly accepted normalized-equivalent lifecycle cases
- rollback when lifecycle verification fails
- rejection when the baseline is not clean
- rejection when the mutation class is deferred or blocked
- detection of silent post-apply lifecycle repair if it appears

This workstream should keep the standing computational, graph, IR,
workbook-facade, and replay baselines green while adding the new lifecycle
authority lane.

Required artifact:

- one checked-in differential surface with explicit verdict categories for
  applied, normalized-equivalent, rolled back, rejected, and repair-detected
  lifecycle cases

### 5.6 Freeze Phase 5 Closeout And The Proceed Decision

Phase 5 closes only with a checked-in decision record that says one of:

- proceed to Phase 6 on the admitted lifecycle-authoritative subset
- proceed only on a narrower lifecycle subset
- stop the computational substrate program here

The closeout must explicitly classify:

- what is now truly engine-authoritative in formula lifecycle
- what still depends on Calc as host, verifier, or lifecycle owner
- whether shared-group lifecycle remains deferred or can partially proceed
- whether execution-IR comparison should remain observational or become a
  harder gate in later phases
- which divergence or repair patterns remain unacceptable

Required artifact:

- one checked-in Phase 5 decision record with explicit proceed or stop
  reasoning

## Target Surfaces

The first implementation sweep for Phase 5 should expect to introduce or
touch:

- new or extended lifecycle state/update code under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- completed substrate surfaces:
  - [ComputationalShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadow.hxx)
  - [DependencyGraphShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx)
  - [ExecutionIr.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIr.hxx)
  - [AuthorityPilot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilot.hxx)
  - [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- Calc compat layers under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- likely new or extended Calc pilot bridges adjacent to:
  - [ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)
  - [RecalcQueueExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx)
- Calc validation lanes under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_compile_diff.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_compile_diff.cxx)
- standalone tests under `spreadsheet_engine/tests/unit/`

## Recommended Execution Order

Phase 5 should run in this order:

1. freeze the lifecycle authority contract and admitted mutation surface
2. define the engine-owned lifecycle state and sync model
3. build the engine-authored lifecycle transition path
4. wire the Calc lifecycle sync and repair-detection bridge
5. add lifecycle differential, rejection, and rollback lanes
6. freeze the Phase 5 decision record

## Validation Contract

Phase 5 validation should keep the existing substrate and replay baselines
green while adding explicit lifecycle-authority coverage.

The minimum closeout contract should be:

- targeted lifecycle-authority cases in `CppunitTest_sc_ucalc_dependency_shadow`
- targeted workbook-facade and compile-diff cases where the lifecycle pilot
  depends on admitted compiler or formula metadata
- a standalone lifecycle or computational-substrate lane under
  `spreadsheet_engine/build_check`
- existing:
  - `spreadsheetengine_computational_graph_tests`
  - `spreadsheetengine_computational_ir_tests`
  - `spreadsheetengine_computational_substrate_tests`
  - `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Where Phase 5 introduces runtime toggles, validation should exercise both:

- accepted authoritative lifecycle application on the admitted subset
- explicit rollback, rejection, or repair-detected outcomes on out-of-contract
  or divergent cases

## Exit Criteria

Phase 5 should be considered complete only if all of the following are true:

1. the engine is the real source of admitted formula lifecycle transitions on
   the narrowed pilot subset
2. the admitted lifecycle mutation set applies with exact or explicitly
   documented normalized-equivalent verification
3. rollback is deterministic, exercised, and not merely theoretical
4. deferred lifecycle classes are rejected explicitly rather than drifting
   into accidental partial authority
5. any silent Calc-side lifecycle repair is either absent on the admitted
   subset or detected and treated as unacceptable
6. standing replay and substrate differential lanes remain green
7. the checked-in decision record states clearly whether Phase 6 should
   proceed, narrow, or stop

Phase 5 should stop rather than proceed if either of these becomes true:

- the engine lifecycle answer still depends on Calc-owned repair logic in ways
  the engine cannot model directly
- rollback, rejection, or repair-detected outcomes become the normal result
  even on the admitted lifecycle subset

## Phase 5 Definition Of Success

Phase 5 is a success if it proves that a narrow engine-authored formula
lifecycle is practical without hiding Calc as the real lifecycle owner.

It is not a success if it merely lets the engine describe lifecycle changes
that Calc then silently rebuilds or repairs on its own.
