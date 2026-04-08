# Computational Substrate Same-Sheet Named-Range Split-Outcome Evidence

Status: completed evidence for the bounded same-sheet named-range
split-outcome pass

## Host-Shape Freeze

This pass froze the actual live Calc contract for the bounded same-sheet
named-range three-group attempt:

- boundary: `GlobalSingleAreaSameSheet`
- mutation family: `Regroup`
- transition kind: `Split`
- after-topology: two groups, with the far participant group left separate

That evidence is covered in:

- [../../../sc/qa/unit/ucalc_workbook_facade.cxx](../../../sc/qa/unit/ucalc_workbook_facade.cxx)

## Standalone Proof

Engine-only exactness proof is covered in:

- [../../tests/unit/computational_substrate_tests.cxx](../../tests/unit/computational_substrate_tests.cxx)

That bucket proves the exact split-backed host shape closes in:

- lifecycle planning
- computational shadow comparison
- dependency-graph comparison
- execution IR comparison

## Live Carry-Through Proof

Live proof is covered in:

- [../../../sc/qa/unit/ucalc_dependency_shadow.cxx](../../../sc/qa/unit/ucalc_dependency_shadow.cxx)

The live buckets now prove:

- direct authority replay stays deferred with exact queue match and graph
  mismatch
- direct lifecycle replay stays deferred with exact queue match, named-range
  match, non-full computational match, and graph mismatch
- mutation-entry applies exactly and lands on the two-group split-backed
  after-topology

## Closeout Meaning

This pass did not discover a real one-group same-sheet collapse family.

Instead it established that:

- the live three-group attempt is a split-backed regroup outcome
- direct authority and lifecycle replay still do not verify exactly on that
  host shape
- mutation-entry already carries the host-shaped regroup outcome exactly

So the practical widening is real, but it is a normalization-backed widening
onto the admitted named-range `Regroup` lane rather than a new distinct
family admission.

## Validation

Validated in this pass:

- `testCalcFacadeSharedGroupNamedRangeThreeGroupAttemptActualHostShape`
- `testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityNamedRangeSplitOutcomeStaysDeferred`
- `testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleNamedRangeSplitOutcomeStaysDeferred`
- `testComputationalMutationEntrySharedGroupNonStructuralNamedRangeSplitOutcomeNormalizesToRegroupApplies`
- `spreadsheetengine_computational_substrate_tests`

Full validation rerun for closeout:

- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Replay remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
