# Computational Substrate Phase 4 Plan

Status: active execution-ready plan for Phase 4

## Purpose

This document is the execution-ready plan for Phase 4 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

Phase 4 begins from the narrowed proceed boundary recorded in
[COMPUTATIONAL_SUBSTRATE_PHASE3_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE3_DECISION_RECORD.md).

Its job is to run the first engine-authoritative pilot for dependency-graph
updates and recalc-queue construction on a safe, IR-backed subset while Calc
still hosts the document, executes the resulting queue, and retains the final
rollback path.

Phase 4 is the first time the computational-substrate program asks the engine
to be more than an exact shadow. It still does not migrate full formula
lifecycle, full listener/broadcaster ownership, or broad structural authority.
It is a deliberately narrow authority pilot.

## Phase 4 Goal

Make `spreadsheet_engine/` authoritative for dependency-graph maintenance and
recalc-queue derivation on the admitted safe mutation subset.

That means the engine must be able to:

- accept the narrowed mutation set on top of the completed computational,
  graph, and IR shadows
- derive the post-mutation dependency and recalc state from engine-owned data
  rather than from Calc as hidden authority
- package an applicable queue/state transition for Calc
- verify the live Calc result after application
- roll back deterministically if the authoritative pilot diverges

Phase 4 is successful only if the engine is the real source of the pilot
graph-and-queue answer on the admitted subset, not merely a second opinion
after Calc has already decided.

## Entry Boundary

Phase 4 should start only from the narrowed subset already validated in
Phase 3:

- engine-owned computational shadow for admitted formula-bearing cells and
  groups
- engine-owned graph shadow for admitted formula cells, groups, listener
  anchors, and broadcaster nodes
- engine-owned execution IR shadow for the admitted formula subset
- deterministic lowering from admitted compiler output into the IR
- exact-or-normalized differential comparison for graph and IR state
- admitted mutation validation for:
  - `SetValue`
  - formula edit
  - formula insertion
  - `ClearCell`
  - named-range rename
  - representative row insert
  - representative column delete

Phase 4 must begin narrower than that full shadow subset. The initial
authoritative pilot surface should be:

- `SetValue`
- scalar `SetString`
- formula text edit via `SetString`
- direct `SetFormula`
- `ClearCell`

The pilot should also require:

- clean formula-tree / recalc baseline before mutation
- no pre-existing dirty authority debt
- no structural edits, copy/move, clipboard, load-time repair, or delayed
  listener edge cases in the authoritative path

Named-range and representative structural cases stay in the validation and
comparison lanes unless Phase 4 evidence later proves they can be admitted
without widening risk prematurely.

## Non-Goals

Phase 4 should not attempt to:

- move full formula lifecycle authority out of Calc
- make listener or broadcaster storage authoritative in the engine yet
- replace Calc execution with direct execution from the engine-owned IR
- migrate broad structural edit handling into the authoritative pilot
- widen into copy, move, clipboard, load-time, or undo authority
- move external-reference cache ownership or other host-heavy document
  services
- leave rollback as a best-effort afterthought

## Required Deliverables

Phase 4 is complete only when all of the following exist:

1. an explicit authority contract for the admitted pilot subset, including
   preconditions, permitted normalized equivalence, and hard stop conditions
2. a stable engine-owned graph-and-queue update surface that produces the
   authoritative pilot answer from admitted pre-mutation state
3. a Calc bridge that applies the pilot answer, verifies the result, and
   rolls back on divergence
4. automated differential validation for exact, normalized-equivalent,
   rollback, and rejected mutation cases
5. explicit evidence for what remains deferred after the pilot
6. a checked-in proceed or stop decision record for Phase 5

## Workstreams

### 4.1 Freeze The Authority Contract And Pilot Mutation Matrix

Freeze the exact Phase 4 authority boundary before implementation widens.

The contract should name:

- the admitted mutation types
- required clean-baseline preconditions
- which graph and queue differences are still considered normalized-equivalent
- which outcomes require rollback immediately
- which mutation classes remain validation-only, deferred, or blocked

This workstream should also define the pilot mutation matrix used by later
tests and runtime guards.

Required artifacts:

- one checked-in Phase 4 authority contract note
- one checked-in pilot mutation matrix or equivalent explicit classification

### 4.2 Define The Engine-Owned Authority State-Transition Model

Define the authority-facing state-transition model the engine will use for the
pilot.

The model should make explicit:

- the pre-mutation shadow inputs required to answer the pilot mutation
- the graph updates and queue outputs produced by the engine
- the verification state that Calc must compare after application
- the rollback triggers and failure categories

Recommended constraints:

- use engine-owned ids from the admitted computational, graph, and IR shadows
- keep Calc pointers, formula-cell addresses, and token-container identity out
  of durable authority state except as adapter inputs or verification aids
- keep rebuild-first authority acceptable if it yields a cleaner real boundary
  than a prematurely incremental Calc-shaped model

Required artifact:

- one checked-in schema or API note for the authoritative pilot state model

### 4.3 Build The Engine-Authoritative Graph And Queue Update Path

Implement the engine-owned update surface that answers the admitted pilot
mutations.

The first implementation may use rebuild-backed authority as long as:

- the engine-owned shadow state is the source of the graph and queue answer
- Calc is not secretly recomputing the answer and only then being compared
- the output is stable enough to drive deterministic verification

This workstream should produce:

- graph-update output for the admitted mutation set
- recalc-plan / queue output for the admitted mutation set
- explicit failure or unsupported verdicts for out-of-scope mutations

Required artifact:

- one checked-in authority update API plus at least one standalone or
  Calc-backed lane that exercises the admitted pilot mutations

### 4.4 Wire The Calc Apply, Verify, And Rollback Bridge

Add the Calc-side bridge that consumes the Phase 4 authority answer.

The bridge should:

- package the live mutation into the admitted pilot input
- apply the engine-derived queue/state transition
- verify the resulting live formula-tree, graph-facing state, and queue
  correspondence
- restore the pre-apply state on divergence
- make accepted fallback or rollback reasons explicit

The bridge should remain opt-in and narrow in this phase.

Required artifact:

- one checked-in compat surface for authoritative apply, verify, and rollback
  on the admitted subset

### 4.5 Add Differential Validation, Rollback, And Rejection Lanes

Phase 4 must prove not only that accepted pilot mutations work, but that
rejected and rollback cases are handled deliberately.

Validation should cover:

- exact graph-and-queue authority on the admitted mutation set
- normalized-equivalent cases that are explicitly accepted by contract
- rollback when verification fails
- skip or reject behavior when the baseline is not clean
- skip or reject behavior for deferred mutation classes

This workstream should keep the standing graph, IR, workbook-facade, and
replay baselines green while adding the new authority lane.

Required artifact:

- one checked-in differential surface with explicit verdict categories for
  applied, normalized-equivalent, rolled back, and rejected pilot cases

### 4.6 Freeze Phase 4 Closeout And The Proceed Decision

Phase 4 closes only with a checked-in decision record that says one of:

- proceed to Phase 5 on the admitted authoritative pilot subset
- proceed only on a narrower subset
- stop the computational substrate program here

The closeout must classify:

- what is now truly engine-authoritative in the pilot
- what still depends on Calc as host, verifier, or lifecycle owner
- which mutation classes remain shadow-only
- which divergence or rollback patterns remain unacceptable

Required artifact:

- one checked-in Phase 4 decision record with explicit proceed or stop
  reasoning

## Target Surfaces

The first implementation sweep for Phase 4 should expect to introduce or
touch:

- new or extended authority state/update code under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- existing completed substrate surfaces:
  - [ComputationalShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadow.hxx)
  - [DependencyGraphShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx)
  - [ExecutionIrBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIrBuilder.hxx)
  - [ExecutionIrMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIrMutation.hxx)
- Calc compat layers under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- existing recalc and queue surfaces, likely including:
  - [RecalcPlanner.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/dependency/RecalcPlanner.hxx)
  - [RecalcAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcAuthority.hxx)
  - [RecalcQueueExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx)
- Calc validation lanes under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_compile_diff.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_compile_diff.cxx)
- standalone tests under `spreadsheet_engine/tests/unit/`

## Recommended Execution Order

Phase 4 should run in this order:

1. freeze the authority contract and mutation matrix
2. define the authority state-transition model
3. build the engine-owned graph-and-queue update path
4. wire the Calc apply, verify, and rollback bridge
5. add differential validation, rollback, and rejection lanes
6. freeze the Phase 4 decision record

## Validation Contract

Phase 4 validation should keep the existing substrate and replay baselines
green while adding explicit authority-pilot coverage.

The minimum closeout contract should be:

- targeted authoritative-pilot cases in `CppunitTest_sc_ucalc_dependency_shadow`
- targeted workbook-facade and compile-diff cases where the pilot depends on
  admitted compiler or formula metadata
- a standalone authority or computational-substrate unit lane under
  `spreadsheet_engine/build_check`
- existing:
  - `spreadsheetengine_computational_graph_tests`
  - `spreadsheetengine_computational_ir_tests`
  - `spreadsheetengine_computational_substrate_tests`
  - `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Where Phase 4 introduces runtime toggles, validation should exercise both:

- accepted authoritative application on the admitted pilot subset
- explicit rollback or rejection on out-of-contract cases

## Exit Criteria

Phase 4 should be considered complete only if all of the following are true:

1. the engine is the real source of graph-and-queue updates on the admitted
   pilot subset
2. the admitted mutation set applies with exact or explicitly documented
   normalized-equivalent verification
3. rollback is deterministic, exercised, and not merely theoretical
4. deferred mutation classes are rejected or skipped explicitly rather than
   drifting into accidental partial authority
5. standing replay and substrate differential lanes remain green
6. the checked-in decision record states clearly whether Phase 5 should
   proceed, narrow, or stop

Phase 4 should stop rather than proceed if either of these becomes true:

- the authoritative answer still depends on Calc-owned repair logic in ways
  the engine cannot model directly
- rollback or rejection becomes the normal outcome even on the admitted pilot
  subset

## Phase 4 Definition Of Success

Phase 4 is a success if it proves that a narrow engine-authoritative pilot is
practical without hiding Calc as the real dependency or recalc authority.

It is not a success if it merely wraps the existing Calc answer in more
adapter code.
