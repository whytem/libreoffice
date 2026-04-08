# Computational Substrate Repair-Sensitive Normalization Evidence

Status: completed evidence summary for the repair-sensitive closeout pass

## Decision Support Summary

This pass did not find a new admitted repair-sensitive family.

What it did prove is narrower and more useful:

- the currently admitted structural families already close exactly on the
  live same-sheet surface
- the current repair-sensitive structural probes are deterministic rollback
  cases with explicit reason codes
- the current repair-sensitive frontier therefore does not hide a real live
  widening target on the bounded same-sheet surface

## Explicit Structural Repair Buckets

The structural pilot now emits explicit reasons instead of one generic
repair string:

- `structural_formula_reference_update_mismatch`
- `structural_named_range_reference_update_mismatch`
- `structural_shared_group_reference_update_mismatch`

These reasons are emitted at the point where the engine-authored predicted
IR after-state no longer matches the observed live IR after-state.

## Standalone Proof

`spreadsheetengine_computational_substrate_tests` now covers the three
explicit structural repair buckets directly:

- plain structural reference-update divergence
- named-range structural reference-update divergence
- shared-group structural reference-update divergence

Each bucket is proven as:

- `StructuralPilotVerdict::RepairDetected`
- rollback-required
- stable explicit reason code

The same standalone suite also continues to cover the already-exact
structural families that remain admitted.

## Live Calc Proof

`CppunitTest_sc_ucalc_dependency_shadow` now proves the same reasons on the
live Calc-backed structural path:

- `testComputationalStructuralValidateGlobalNamedRangeRepairDetected`
- `testComputationalStructuralValidateSharedGroupRepairDetected`
- `testComputationalStructuralRepairDetectedRollback`
- `testComputationalStructuralDeleteRowRepairDetectedRollback`
- `testComputationalStructuralInsertColumnRepairDetectedRollback`

The live results show:

- named-range structural divergence closes as
  `structural_named_range_reference_update_mismatch`
- shared-group structural divergence closes as
  `structural_shared_group_reference_update_mismatch`
- plain structural divergence closes as
  `structural_formula_reference_update_mismatch`

## Interpretation

The important architectural result is that the current repair-sensitive
surface no longer looks like hidden host cleanup.

On the bounded same-sheet surface, the observed repair-sensitive cases are
better understood as:

- exact families that are already admitted when the after-state is exact
- explicit deterministic rollback when the observed after-state diverges from
  the engine-authored reference update
- explicit reject-by-rule outside the exact slice

That means repair-sensitive normalization no longer stands as a distinct
top-level widening program on the current roadmap.

## Validation Used For Closeout

- `spreadsheetengine_computational_substrate_tests`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

