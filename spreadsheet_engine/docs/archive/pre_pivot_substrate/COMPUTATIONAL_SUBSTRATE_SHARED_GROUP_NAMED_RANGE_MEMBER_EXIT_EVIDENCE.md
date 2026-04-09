# Computational Substrate Shared-Group Named-Range Member-Exit Evidence

Status: frozen evidence for bounded named-range-combined scalar member-exit admission

## Outcome

The bounded scalar member-exit family now closes exactly in standalone and
live proof.

The broader non-scalar member-exit frontier remains deferred in this pass.

## Validation Sweep

The closeout validation sweep passed with:

- `make -j1 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_workbook_facade`
- `make -j1 CppunitTest_sc_ucalc_compile_diff`
- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_graph_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Representative proof buckets now covered:

- `testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityNamedRangeMemberExitApplies`
- `testComputationalMutationEntrySharedGroupNonStructuralNamedRangeMemberExitAuthorityApplies`
- `testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityNamedRangeMemberExitClearCellStaysRejected`
- `testComputationalMutationEntrySharedGroupNonStructuralNamedRangeMemberExitAuthorityClearCellStaysRejected`

The bounded non-scalar `SetFormula` lane was closed later in the follow-on
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_DECISION_RECORD.md).

## Replay Baseline

Replay remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
