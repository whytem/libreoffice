# Computational Substrate Shared-Group Non-Structural Named-Range-Combined Evidence

Status: frozen evidence note for the bounded named-range-combined closeout

## Facade Proof

The facade layer now proves:

- bounded same-sheet global single-area named-range-combined
  `SameTextPreserve` is recognized explicitly
- off-sheet named-range consumers are recognized explicitly and deferred

Coverage:

- [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)
- [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)

## Standalone Proof

The standalone substrate tests now prove:

- exact named-range-combined `SameTextPreserve` closes computational, graph,
  and IR state
- bounded named-range-combined member-exit stays rejected with
  `shared_group_named_range_out_of_contract`

Coverage:

- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)

## Live Proof

The live Calc proof now shows:

- lifecycle keeps bounded named-range-combined `SameTextPreserve` deferred
- mutation entry keeps bounded named-range-combined `SameTextPreserve`
  deferred and reports `listener_anchor_out_of_contract`
- authority rejects bounded named-range-combined member-exit
- mutation entry rejects bounded named-range-combined member-exit

Coverage:

- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Validation Sweep

The closeout validation sweep passed with:

- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `make -j1 CPPUNIT_TEST_NAME=testCalcFacadeSharedGroupNamedRangeBoundarySameTextPreserve CppunitTest_sc_ucalc_workbook_facade`
- `make -j1 CPPUNIT_TEST_NAME=testCalcFacadeSharedGroupNamedRangeBoundaryOffSheetStaysDeferred CppunitTest_sc_ucalc_workbook_facade`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityNamedRangeMemberExitStaysRejected CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleNamedRangeSameTextPreserveStaysDeferred CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralNamedRangeMemberExitAuthorityStaysRejected CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralNamedRangeSameTextPreserveLifecycleStaysDeferred CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_compile_diff`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_graph_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Replay remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## Closeout Result

This evidence supports a no-admit closeout for this cycle.

It proves:

- standalone exactness exists for the bounded preserve candidate
- live lifecycle and mutation-entry carry-through do not yet admit that
  candidate
- bounded member-exit also remains deferred
