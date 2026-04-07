# Computational Substrate Shared-Group Non-Structural Multi-Group Collapse Evidence

Status: frozen evidence note for the exact multi-group collapse closeout

## Proof Buckets

The multi-group collapse cycle required proof in five buckets:

- facade-side synthetic multi-group-collapse classification
- standalone exact synthetic three-participant collapse
- standalone retained four-plus-group reject
- Calc facade proof of the live host after-topology
- live lifecycle and mutation-entry proof that the far group stays separate

## Landed Coverage

The checked-in proof surface is:

- synthetic multi-group-collapse classification in
  [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)
- standalone exact synthetic three-participant collapse and four-plus-group
  reject in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- Calc facade proof that a live three-group attempt leaves the far group
  separate in
  [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
- live lifecycle and mutation-entry proof of the same retained far-group
  separation in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Decision-Level Evidence

This evidence note shows:

- the engine can author the bounded full-span collapse model exactly in
  standalone proof
- current live Calc behavior does not expose that same after-topology
- therefore the cycle does not justify an admitted-slice expansion

## Validation

The multi-group-collapse validation set executed in this cycle is:

- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `make -j1 CPPUNIT_TEST_NAME=testCalcFacadeSharedGroupThreeGroupAttemptKeepsFarGroupSeparate CppunitTest_sc_ucalc_workbook_facade`
- `make -j1 CPPUNIT_TEST_NAME=testCalcFacadeSharedGroupMutationClassificationSameTextPreserve CppunitTest_sc_ucalc_workbook_facade`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleThreeGroupAttemptKeepsFarGroupSeparate CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralThreeGroupAttemptKeepsFarGroupSeparate CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleMerge CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleReplacementMerge CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleOneSidedInsert CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralMergeLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralReplacementMergeLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralOneSidedInsertLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_compile_diff`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_graph_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Remaining Deferred Evidence Buckets

This evidence note does not establish admission for:

- bounded three-participant live full-span collapse
- four-plus-group collapse
- named-range-combined collapse
- repair-sensitive normalization
- off-sheet collapse
- broader non-edge regroup or merge
