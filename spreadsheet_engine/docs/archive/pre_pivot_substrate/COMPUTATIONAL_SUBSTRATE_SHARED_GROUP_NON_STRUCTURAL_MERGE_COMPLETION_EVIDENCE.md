# Computational Substrate Shared-Group Non-Structural Merge Completion Evidence

Status: frozen evidence note for the same-sheet merge-completion closeout

## Proof Buckets

The merge-completion closeout required proof in five buckets:

- workbook-facade one-sided insert classification
- standalone exact one-sided insert prediction
- live lifecycle one-sided insert apply
- live mutation-entry one-sided insert apply
- retained broader multi-group reject coverage

## Landed Coverage

The checked-in proof surface is:

- standalone one-sided insert classification in
  [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)
- standalone exact one-sided insert lifecycle closure and broader
  multi-group rejection in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- live lifecycle one-sided insert proof in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- live mutation-entry one-sided insert proof in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- retained named-range, repair-sensitive, and off-sheet reject coverage in
  the earlier non-structural frontier, regroup, merge, and replacement-merge
  evidence buckets

## Exactness Claim

The newly admitted family is exact for the bounded one-sided insert slice
because:

- the participant group and rebuild window are engine-authored
- the inserted formula cell is created in the predicted shadow before group
  rebuild
- shared-group bindings are rebuilt from lowered formulas, not copied from
  live Calc after-topology
- admission still requires an exact bounded observed-after group match
- lifecycle and mutation-entry close through exact computational, queue,
  graph, and IR verification

## Validation

The merge-completion validation set executed in this cycle is:

- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleOneSidedInsert CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralOneSidedInsertLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleMerge CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleReplacementMerge CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralMergeLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralReplacementMergeLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testCalcFacadeSharedGroupMutationClassificationSameTextPreserve CppunitTest_sc_ucalc_workbook_facade`
- `make -j1 CppunitTest_sc_ucalc_compile_diff`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_graph_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Remaining Deferred Evidence Buckets

This evidence note does not establish admission for:

- multi-group collapse beyond the current two-participant merge families
- named-range-combined merge
- repair-sensitive normalization
- off-sheet merge widening
- broader non-edge regroup or merge
