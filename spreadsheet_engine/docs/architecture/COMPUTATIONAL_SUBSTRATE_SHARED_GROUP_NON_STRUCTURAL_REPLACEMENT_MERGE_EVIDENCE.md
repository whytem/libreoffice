# Computational Substrate Shared-Group Non-Structural Replacement-Merge Evidence

Status: frozen evidence note for the exact replacement-merge closeout

## Proof Buckets

The replacement-merge closeout required proof in five buckets:

- workbook-facade replacement-merge classification
- standalone exact replacement-merge prediction
- live lifecycle replacement-merge apply
- live mutation-entry replacement-merge apply
- retained broader-merge rejection

## Landed Coverage

The checked-in proof surface is:

- standalone replacement-merge classification in
  [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)
- standalone exact replacement-merge lifecycle closure and broader-merge
  rejection in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- live lifecycle replacement-merge proof in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- live mutation-entry replacement-merge proof in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- retained one-sided insert rejection coverage in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
  and
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Exactness Claim

The newly admitted family is exact for the bounded edge replacement-merge
slice because:

- the merge participants and rebuild window are engine-authored
- shared-group bindings are rebuilt from lowered formulas, not copied from
  live Calc after-topology
- admission still requires an exact bounded observed-after group match
- lifecycle and mutation-entry close through exact computational, queue,
  graph, and IR verification

## Validation

The replacement-merge closeout validation set executed in this cycle is:

- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleReplacementMerge CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralReplacementMergeLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleMerge CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleOneSidedExtensionStaysRejected CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralMergeLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralOneSidedExtensionStaysRejected CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_compile_diff`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_graph_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Remaining Deferred Evidence Buckets

This evidence note does not establish admission for:

- one-sided adjacent insertion
- multi-group collapse
- named-range-combined replacement merge
- repair-sensitive normalization
- off-sheet replacement merge widening
- broader non-edge regroup or merge
