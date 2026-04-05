# Computational Substrate Object Realization Evidence

Status: frozen object-realization evidence

## Purpose

This note records the bounded proof results for
[COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md)
after the landed observation and engine-authored realization workstreams.

The question for this cycle is whether admitted live object realization can
be made more explicitly engine-authored without weakening the exact proof
contract on the admitted slice.

## Exact Differential Results

The landed exact proof lanes are:

- `testComputationalObjectRealizationClassifierKinds`
- `testComputationalObjectRealizationObservationExactState`
- `testComputationalMutationEntrySetValue`
- `testComputationalMutationEntrySetFormula`
- `testComputationalMutationEntryInsertRows`

The admitted exact result is:

- the engine-authored `AdmittedObjectRealization` record realizes the
  admitted exact lifecycle case successfully
- direct admitted mutation-entry application now carries an explicit
  `moObjectRealizationObservation`
- applied admitted mutation-entry results now close with
  `ObjectRealizationObservationKind::Exact`

That means the live object-realization layer is no longer just implied by
three separate resident stores at the main call sites. It is now driven by a
single engine-authored realization record on the admitted slice.

## Missing-Object Differential Result

The bounded negative proof lane is:

- `testComputationalObjectRealizationObservationClassifiesMissingObjects`

That lane deliberately removes one realized admitted formula object after the
engine-authored realization step and confirms the observation layer reports:

- `ObjectRealizationObservationKind::MissingRealizedObjects`

This matters because it shows the new observation path can distinguish a true
live realization loss from:

- pure ordering drift
- broadcaster-only host reconstruction
- generic queue or graph mismatch

## Rollback And Repair Surface

The object-realization workstream does not move rollback out of Calc.

The retained rollback and repair boundary remains:

- dirty-baseline rejection stays in the admitted mutation-entry path
- verification failure still rolls back through the captured before-state
- repair-detected results remain rollback-triggering outcomes rather than
  tolerated normalization

The new object-realization record is used for both:

- realized after-state application
- rollback re-realization of the captured before-state

So this workstream shrinks hidden host realization authority without
pretending the rollback host has already moved.

## Replay Baseline

The standing replay baseline remains exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## Bounded Runtime Sample

One focused runtime sample was captured on the exact object-realization proof
lane:

- command:
  - `CPPUNIT_TEST_NAME=testComputationalObjectRealizationObservationExactState make -j1 CppunitTest_sc_ucalc_dependency_shadow`
- sample result:
  - `elapsed=7.70s`
  - `max_rss_kb=248620`

This is only a bounded proof-lane sample, not a full rollout benchmark, but
it confirms the new object-realization layer does not immediately introduce a
gross runtime or memory regression on the exercised admitted slice.

## Validation Set Kept Green

The evidence for this cycle was produced with the following green:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Evidence Reading

The evidence supports one bounded conclusion:

- admitted live object realization is now materially more explicit and
  engine-authored on the admitted slice

It does not, by itself, support a claim that Calc no longer matters at all
for live realization or rollback. The remaining question for closeout is
whether that narrower host role is now strong enough to count as a settled
boundary shift or should still be described as a hybrid host path.
