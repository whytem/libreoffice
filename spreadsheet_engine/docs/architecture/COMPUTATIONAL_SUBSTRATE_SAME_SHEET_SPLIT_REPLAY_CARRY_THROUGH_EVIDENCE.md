# Computational Substrate Same-Sheet Split Replay Carry-Through Evidence

Status: completed evidence for the direct same-sheet split-backed replay
pass

## Admitted Result

This pass admitted the exact bounded same-sheet named-range split-backed
three-group `SetFormula` replay lane on the
`GlobalSingleAreaSameSheet` surface.

The live family remains:

- shared-formula mutation family `Regroup`
- shared-formula transition kind `Split`

## Mismatch Isolation

The pass proved that the old retained mismatch had become very narrow.

Before the replay-shaping fix:

- queue comparison was exact
- execution IR comparison was exact
- formula groups, listener anchors, broadcaster nodes, and named ranges
  already matched
- only broadcaster attachment and graph edges still mismatched

That isolated the blocker to the named-range area-broadcaster replay shape.

## Runtime Fix

The bounded replay fix landed in:

- [../../inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx](../../inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)

It introduces exact split-backed named-range replay shaping for the bounded
same-sheet surface:

- touched surviving after-group replays as top-cell plus touched-cell
  area listeners
- far surviving after-group replays as group listener plus top-cell
  augmentation

## Proof

Live proof is covered in:

- [../../../sc/qa/unit/ucalc_dependency_shadow.cxx](../../../sc/qa/unit/ucalc_dependency_shadow.cxx)
- [../../../sc/qa/unit/ucalc_workbook_facade.cxx](../../../sc/qa/unit/ucalc_workbook_facade.cxx)

Standalone exactness proof remains in:

- [../../tests/unit/computational_substrate_tests.cxx](../../tests/unit/computational_substrate_tests.cxx)

The updated live buckets now prove:

- `testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityNamedRangeSplitOutcomeApplies`
- `testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleNamedRangeSplitOutcomeApplies`
- `testComputationalMutationEntrySharedGroupNonStructuralNamedRangeSplitOutcomeNormalizesToRegroupApplies`
- `testCalcFacadeSharedGroupNamedRangeThreeGroupAttemptActualHostShape`

## Validation

Validated in this pass:

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
