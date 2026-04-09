# Computational Substrate Shared-Group Widening Plan

Status: complete closeout record

## Purpose

This document defines the next bounded widening cycle after
[ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md)
closed current-slice ownership.

The goal is narrow and explicit:

- widen the ownership-complete admitted slice into the next adjacent
  workbook class
- keep the current admitted mutation set fixed while shared-group behavior
  is reassessed
- avoid mixing shared-group widening with named-range-sensitive, sheet-wide,
  or broad document-flow widening

This is not a plan for named-range-sensitive structural behavior, sheet
insert/delete/rename/move, copy/move/clipboard/load-time/undo-like flows, or
broad `ScDocument` independence. It is the next workbook-class widening step
on top of the ownership-complete admitted slice.

This plan is now complete.

Its closeout result is:

- widen the live rollout on one bounded structural family:
  exact same-sheet shareable shared-group `Preserve`, `Split`, and `Rebuild`
  cases
- keep non-exact or non-structural shared-group behavior validation-only or
  deferred
- keep broader shared-group widening out of live admission until exact
  engine-predicted topology extends beyond that structural family

## Why This Plan Exists

The current admitted slice is now substantively ownership-complete on the
bounded ordinary-scalar-formula subset.

What still stays outside that slice is no longer another host-ownership seam.
It is broader workbook behavior, especially classes that were repeatedly
deferred earlier because they require formula-group lifecycle and repair
answers.

The closest adjacent deferred workbook class is:

- workbooks containing shared formula groups

This is the best next candidate because:

- the rollout matrix already marks shared-group-containing workbooks as the
  nearest adjacent `pilot-only` class
- named-range-sensitive structural behavior was reassessed twice and still
  failed live admission
- the engine already exposes shared-formula inspection surfaces through the
  workbook facade
- widening shared-group behavior keeps the mutation vocabulary fixed while
  broadening the workbook class, which is the right next move after
  ownership closeout

## Plan Goal

Determine whether `spreadsheet_engine` can safely widen the current
ownership-complete admitted slice to include bounded shared-group lifecycle
and repair behavior on the already-admitted mutation set.

The closeout must answer one explicit question:

- shared-group-containing workbooks can become part of the admitted slice on
  a bounded subset
- shared-group behavior remains validation-only or hybrid for one bounded
  reason
- or shared-group widening must be deferred again

## Entry Boundary

This plan begins from the current settled boundary:

- the first-stage extraction boundary is complete and stable
- the computational-substrate authority program closed with a narrow proceed
  result
- the admitted narrow rollout remains bounded to the scalar and single-sheet
  structural slice
- the admitted slice is now substantively ownership-complete on the bounded
  no-shared-group subset
- exact queue, computational, graph, replay, realization, rollback, and
  verification already hold on that ownership-complete slice
- shared-group-sensitive workbook behavior remains explicitly deferred

This plan therefore treats shared-group lifecycle and repair as the next
bounded widening surface rather than as another ownership-completion seam.

## In-Scope Widening Surface

The widening surface remains intentionally narrow.

Shared-group reassessment is limited to:

- ordinary Calc workbooks that contain shared formula groups
- the already-admitted mutation vocabulary:
  - `SetScalarValue`
  - `SetFormula`
  - `ClearCell`
  - single-sheet `InsertRows`
  - single-sheet `DeleteRows`
  - single-sheet `InsertColumns`
  - single-sheet `DeleteColumns`
- clean baseline only
- single-sheet shared-group preserve, split, rebuild, and repair behavior
- formula-group lifecycle, listener/broadcaster wiring, and formula-tree or
  formula-track effects caused by those shared-group transitions

The widening cycle may admit only a subset of those cases. It does not need
to prove all shared-group behavior at once.

## Non-Goals

This plan should not attempt to:

- widen into named-range-sensitive structural behavior
- widen into sheet insert, delete, rename, or move
- widen into copy, move, clipboard, load-time, or undo-like flows
- broaden the admitted mutation vocabulary beyond the existing scalar and
  single-sheet structural classes
- reopen already-settled ownership of resident storage, resident wiring,
  lifetime, mutation-entry, realization, rollback, verification, primitive
  execution, or host-call identity on the current admitted slice
- weaken exact queue, computational, graph, replay, realization, rollback,
  or verification requirements
- claim broad `ScDocument` independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the shared-group widening boundary
2. a checked-in scenario matrix covering representative shared-group
   preserve, split, rebuild, and repair classes
3. a checked-in mapping and equivalence note for shared-group identity,
   normalization, and forbidden host shortcuts
4. a checked-in implementation note for the bounded shared-group pilot path
5. a checked-in evidence note covering exact, reject, rollback, repair, and
   deferred shared-group outcomes
6. a checked-in decision record saying whether bounded shared-group behavior:
   - is admitted into the current slice
   - remains validation-only or hybrid for one bounded reason
   - or is deferred again

## Workstreams

### 1. Freeze The Shared-Group Widening Contract

Freeze the exact widening surface before implementation work begins.

This contract should name:

- the admitted mutation classes carried forward from the ownership-complete
  slice
- the shared-group workbook classes under reassessment:
  - preserve-existing-group cases
  - split-group cases
  - rebuild-group cases
  - repair-detected or host-repair-sensitive cases
- retained Calc-owned host surfaces that stay out of scope in this cycle
- the explicit definition of success versus hybrid or defer for
  shared-group widening

Required artifact:

- one checked-in shared-group widening contract note

### 2. Freeze The Shared-Group Scenario Matrix

Define the representative shared-group cases the proof cycle must cover.

This matrix should classify:

- admitted-candidate shared-group cases
- validation-only shared-group cases
- immediately rejected shared-group cases

It should include representative examples for:

- scalar overwrite into a shared-group consumer or member
- formula replace on a shared-group anchor or member
- `ClearCell` against a shared-group member
- row or column edits that preserve, split, or rebuild a same-sheet shared
  group
- cases that force retained host-only repair and therefore stay deferred

Required artifact:

- one checked-in shared-group scenario matrix

### 3. Freeze The Shared-Group Mapping And Equivalence Rules

Define the stable identity and equivalence rules for shared-group behavior.

This note should define:

- stable shared-group anchor identity
- group length and membership identity
- shareable versus non-shareable group classification
- equivalence rules for rebuilt or normalized shared-group outcomes
- forbidden Calc-local shortcuts that would reclaim shared-group authority

Required artifact:

- one checked-in shared-group mapping and equivalence note

### 4. Build The Validation-Only Shared-Group Pilot Path

Implement the narrowest runtime path needed to exercise shared-group behavior
through the existing ownership-complete slice.

This workstream should:

- extend shared-group detection and summary surfaces on the workbook facade
- add bounded shared-group pilot lanes to mutation entry and differential
  validation
- surface shared-group preserve, split, rebuild, and repair outcomes
  explicitly in test output
- keep shared-group widening scoped only to the admitted mutation set

Required artifact:

- one checked-in implementation note for the shared-group pilot path

### 5. Freeze Differential Shared-Group Widening Evidence

Run the bounded shared-group proof cycle and record the results.

This evidence note should summarize:

- exact admitted-candidate shared-group outcomes
- reject, rollback, repair-detected, and deferred outcomes
- whether resident cell state, resident wiring state, lifetime,
  realization, rollback, and verification still close exactly on any
  admitted-candidate shared-group slice
- memory and performance observations
- whether shared-group-containing workbooks now fit inside a meaningful
  widened slice

Required artifact:

- one checked-in shared-group widening evidence note

### 6. Freeze The Shared-Group Widening Decision

Close the cycle with one explicit decision:

- bounded shared-group behavior is now admitted
- shared-group widening remains validation-only or hybrid for one bounded
  reason
- or shared-group widening is deferred again

The closeout must also name the next adjacent concern after this cycle.

Required artifact:

- one checked-in shared-group widening decision record

## Target Surfaces

The most likely target surfaces are:

- [WorkbookFacade.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/WorkbookFacade.hxx)
- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

If shared-group widening proves to depend on host surfaces not representable
through those seams, the cycle should stop and record that boundary
explicitly instead of widening by implication.

## Recommended Execution Order

The recommended order is:

1. freeze the shared-group widening contract
2. freeze the shared-group scenario matrix
3. freeze the shared-group mapping and equivalence rules
4. implement the validation-only shared-group pilot path
5. freeze differential shared-group widening evidence
6. freeze the shared-group widening decision

This order keeps the widening cycle honest: define the bounded proof surface
first, then implement only enough runtime support to answer that question.

## Validation Contract

At minimum, each closeout phase should keep these green:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

The standing replay baseline must remain exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## Exit Criteria

This plan is complete only if all of the following are true:

- the contract, matrix, mapping, implementation, evidence, and decision
  artifacts all exist
- the shared-group proof cycle produces explicit admitted, rejected,
  rollback, repair, or deferred outcomes
- the closeout clearly says whether shared-group-containing workbooks become
  part of the admitted slice
- any retained hybrid or defer reason is narrow and explicit
- the validation contract stays green
- the zero-fallback replay baseline remains exact
