# Computational Substrate Shared-Group Non-Structural Merge Evidence

Status: frozen evidence note for the exact merge closeout

## Proof Buckets

The merge closeout required proof in five buckets:

- workbook-facade merge classification
- standalone exact merge prediction
- live lifecycle merge apply
- live mutation-entry merge apply
- retained broader-merge rejection

## Landed Coverage

The checked-in proof surface is:

- standalone merge classification in
  [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)
- standalone exact merge lifecycle closure in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- live lifecycle merge proof in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- live mutation-entry merge proof in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- retained one-sided insert rejection coverage in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
  and
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Exactness Claim

The newly admitted family is exact for the bounded gap-closing merge slice
because:

- the merge window is engine-authored
- the inserted formula cell is created in the predicted shadow before group
  rebuild
- shared-group bindings are rebuilt from lowered formulas, not copied from
  live Calc after-topology
- admission still requires an exact observed-after topology match
- lifecycle and mutation-entry close through exact computational, queue,
  graph, and IR verification

## Validation

The merge closeout validation set executed in this cycle is:

- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_graph_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `make -j1 CPPUNIT_TEST_NAME=testCalcFacadeSharedGroupMutationClassificationSameTextPreserve CppunitTest_sc_ucalc_workbook_facade`
- `make -j1 CppunitTest_sc_ucalc_compile_diff`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleMerge CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleOneSidedExtensionStaysRejected CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralMergeLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralOneSidedExtensionStaysRejected CppunitTest_sc_ucalc_dependency_shadow`
- `git diff --check`

## Remaining Deferred Evidence Buckets

This evidence note does not establish admission for:

- one-sided adjacent insertion
- replacement-driven merge
- multi-group merge collapse
- named-range-combined merge
- repair-sensitive normalization
- off-sheet merge widening
