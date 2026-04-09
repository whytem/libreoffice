# Computational Substrate Shared-Group Named-Range Non-Scalar Member-Exit Evidence

Status: frozen evidence note for bounded named-range-combined non-scalar
member-exit closeout

## Standalone Proof

- `spreadsheetengine_computational_substrate_tests`
  includes exact bounded named-range-combined `SetFormula`
  `MemberExit` proof

## Live Calc Proof

- `testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityNamedRangeMemberExitSetFormulaApplies`
- `testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleNamedRangeMemberExitSetFormulaApplies`
- `testComputationalMutationEntrySharedGroupNonStructuralNamedRangeMemberExitLifecycleSetFormulaApplies`

## Retained Reject Proof

- `testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityNamedRangeMemberExitClearCellStaysRejected`
- `testComputationalMutationEntrySharedGroupNonStructuralNamedRangeMemberExitAuthorityClearCellStaysRejected`

## Validation Commands

- `make -j1 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_workbook_facade`
- `make -j1 CppunitTest_sc_ucalc_compile_diff`
- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_graph_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`
