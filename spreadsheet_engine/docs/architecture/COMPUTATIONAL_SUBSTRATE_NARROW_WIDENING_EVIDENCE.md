# Computational Substrate Narrow Widening Evidence

Status: active widening evidence note for the narrow rollout plan

## Purpose

This note records the checked-in evidence for the two first widening
candidates in the narrow rollout plan:

- `DeleteRows`
- `InsertColumns`

The goal is to classify each candidate honestly against the frozen proof
threshold in
[COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md),
without changing the live rollout boundary yet.

## Evidence Summary

Both widening candidates currently satisfy the required evidence shape on the
narrow workbook slice:

- exact standalone prediction for structural transition and reference updates
- exact Calc differential happy-path application
- deterministic dirty-baseline rejection
- deterministic out-of-slice rejection
- deterministic repair-detected rollback when divergence is injected

No happy-path candidate case in this note required normalized-equivalent
classification or hidden Calc repair.

## DeleteRows

### Standalone And Substrate Evidence

- `spreadsheetengine_computational_substrate_tests`
  - contract classification remains `ValidationOnly`
  - `buildStructuralPilotTransition(..., Validation)` produces an exact
    applicable transition for the admitted scalar slice
- `spreadsheetengine_computational_ir_tests`
  - delete-row reference-update lowering rewrites the surviving scalar
    reference exactly as expected

### Calc Differential Evidence

- exact happy path:
  - `testComputationalStructuralDeleteRowValidationCandidate`
- dirty-baseline rejection:
  - `testComputationalStructuralDeleteRowCandidateRejectsDirtyBaseline`
- out-of-slice rejection:
  - `testComputationalStructuralDeleteRowCandidateRejectsNamedRangeSlice`
- repair-detected rollback:
  - `testComputationalStructuralDeleteRowCandidateRepairDetectedRollback`

### Classification

`DeleteRows` is currently:

- promotable on the same narrow scalar structural slice already admitted for
  `InsertRows` and `DeleteColumns`

Reason:

- the happy-path differential case lands as `Applied`
- queue verification is exact
- computational verification is exact
- graph verification is exact
- reference-update expectations match the live after-state
- rejection and rollback paths remain deterministic

## InsertColumns

### Standalone And Substrate Evidence

- `spreadsheetengine_computational_substrate_tests`
  - contract classification remains `ValidationOnly`
  - `buildStructuralPilotTransition(..., Validation)` produces an exact
    applicable transition for the admitted scalar slice
- `spreadsheetengine_computational_ir_tests`
  - insert-column reference-update lowering rewrites the surviving scalar
    reference exactly as expected

### Calc Differential Evidence

- exact happy path:
  - `testComputationalStructuralInsertColumnValidationCandidate`
- dirty-baseline rejection:
  - `testComputationalStructuralInsertColumnCandidateRejectsDirtyBaseline`
- out-of-slice rejection:
  - `testComputationalStructuralInsertColumnCandidateRejectsNamedRangeSlice`
- repair-detected rollback:
  - `testComputationalStructuralInsertColumnCandidateRepairDetectedRollback`

### Classification

`InsertColumns` is currently:

- promotable on the same narrow scalar structural slice already admitted for
  `InsertRows` and `DeleteColumns`

Reason:

- the happy-path differential case lands as `Applied`
- queue verification is exact
- computational verification is exact
- graph verification is exact
- reference-update expectations match the live after-state
- rejection and rollback paths remain deterministic

## Interpretation

This note does not itself widen the live rollout.

What it establishes is narrower and more useful:

- both candidates are stronger than mere validation-only hypotheses
- neither candidate currently shows a proof-gap on the admitted scalar slice
- the later rollout decision workstream may now choose to promote neither,
  either, or both based on bounded rollout evidence rather than lack of
  structural proof

## Standing Validation

The evidence above is backed by:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

