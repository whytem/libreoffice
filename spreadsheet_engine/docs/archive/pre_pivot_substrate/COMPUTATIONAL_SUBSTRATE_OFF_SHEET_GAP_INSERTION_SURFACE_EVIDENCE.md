# Computational Substrate Off-Sheet Gap Insertion Surface Evidence

Status: completed evidence for the bounded direct off-sheet gap-closing
insertion pass

## Admitted Result

This pass admitted the exact bounded same-workbook one-consumer-sheet direct
off-sheet gap-closing insertion `SetFormula` lane.

The live workbook-facade host shape remains:

- merged after-topology on the mutation sheet
- mutation-family `None`

That host-shaped lane is now admitted because the direct authority queue
surface and the existing lifecycle and mutation-entry surfaces all close
exactly on the same bounded scenario.

## Proof

Live host-shape proof remains in:

- [../../../sc/qa/unit/ucalc_workbook_facade.cxx](../../../sc/qa/unit/ucalc_workbook_facade.cxx)

The bounded direct off-sheet authority proof added in this pass is in:

- [../../../sc/qa/unit/ucalc_dependency_shadow.cxx](../../../sc/qa/unit/ucalc_dependency_shadow.cxx)

That authority bucket now proves:

- the host-shaped direct off-sheet gap insertion is applicable
- the predicted recalc queue includes both the inserted formula cell and the
  off-sheet consumer
- graph and IR carry-through remain exact

The already-existing live carry-through proof remains in:

- [../../../sc/qa/unit/ucalc_dependency_shadow.cxx](../../../sc/qa/unit/ucalc_dependency_shadow.cxx)

Those buckets already prove:

- lifecycle direct off-sheet gap insertion applies
- mutation-entry direct off-sheet gap insertion applies

Standalone exactness remains in:

- [../../tests/unit/computational_substrate_tests.cxx](../../tests/unit/computational_substrate_tests.cxx)

## Runtime Outcome

No production runtime patch survived this pass.

The deciding result was proof-based:

- the bounded direct off-sheet lane was already exact on the existing
  admitted machinery
- the missing piece was the bounded authority proof and the correct
  statement of the authority contract for this host-shaped surface

## Validation

Validated in this pass:

- `testCalcFacadeSharedGroupOffSheetMergeAttemptActualHostShape`
- `testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityOffSheetConsumerGapInsertApplies`
- `testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleOffSheetConsumerMergeApplies`
- `testComputationalMutationEntrySharedGroupNonStructuralOffSheetConsumerMergeLifecycleApplies`
- `spreadsheetengine_computational_substrate_tests`

The full closeout validation also includes:

- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Replay remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
