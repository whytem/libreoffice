# Computational Substrate Narrow Widening Evidence

Status: complete widening evidence note for the narrow rollout plan

## Purpose

This note records the checked-in evidence for the two first widening
candidates in the narrow rollout plan:

- `DeleteRows`
- `InsertColumns`

The goal of this completed evidence pass was to classify each candidate
honestly against the frozen proof threshold in
[COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md),
before the rollout decision widened the live boundary.

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
  - the structural builder produced an exact
    applicable transition for the admitted scalar slice
- `spreadsheetengine_computational_ir_tests`
  - delete-row reference-update lowering rewrites the surviving scalar
    reference exactly as expected

### Calc Differential Evidence

- exact happy path:
  - `testComputationalStructuralDeleteRowPilot`
- dirty-baseline rejection:
  - `testComputationalStructuralDeleteRowRejectsDirtyBaseline`
- out-of-slice rejection:
  - `testComputationalStructuralDeleteRowRejectsNamedRangeSlice`
- repair-detected rollback:
  - `testComputationalStructuralDeleteRowRepairDetectedRollback`

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
  - the structural builder produced an exact
    applicable transition for the admitted scalar slice
- `spreadsheetengine_computational_ir_tests`
  - insert-column reference-update lowering rewrites the surviving scalar
    reference exactly as expected

### Calc Differential Evidence

- exact happy path:
  - `testComputationalStructuralInsertColumnPilot`
- dirty-baseline rejection:
  - `testComputationalStructuralInsertColumnRejectsDirtyBaseline`
- out-of-slice rejection:
  - `testComputationalStructuralInsertColumnRejectsNamedRangeSlice`
- repair-detected rollback:
  - `testComputationalStructuralInsertColumnRepairDetectedRollback`

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

This note supplied the proof used by the later rollout decision to widen the
live structural rollout surface by one bounded step.

## Standing Validation

The evidence above is backed by:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
