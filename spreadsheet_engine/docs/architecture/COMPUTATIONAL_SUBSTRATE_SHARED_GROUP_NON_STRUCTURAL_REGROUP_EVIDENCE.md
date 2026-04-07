# Computational Substrate Shared-Group Non-Structural Regroup Evidence

Status: frozen evidence note for the exact regroup closeout

## Proof Buckets

The regroup closeout required proof in four buckets:

- workbook-facade regroup classification
- standalone exact regroup prediction
- live lifecycle regroup apply
- live mutation-entry regroup apply

## Landed Coverage

The checked-in proof surface is:

- standalone regroup classification in
  [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)
- standalone exact regroup lifecycle closure in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- live lifecycle regroup proof in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- live mutation-entry regroup proof in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- retained merge rejection coverage in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
  and
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Exactness Claim

The newly admitted family is exact for the bounded edge-regroup slice
because:

- the regroup window is engine-authored
- shared-group bindings are rebuilt from lowered formulas, not copied from
  live Calc after-topology
- admission still requires an exact observed-after topology match
- lifecycle and mutation-entry close through exact computational, queue,
  graph, and IR verification

## Validation

The regroup closeout validation set executed in this cycle is:

- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleRegroup CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralRegroupLifecycle CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralMergeStaysRejected CppunitTest_sc_ucalc_dependency_shadow`
- `git diff --check`

## Remaining Deferred Evidence Buckets

This evidence note does not establish admission for:

- merge
- named-range-combined regroup
- repair-sensitive normalization
- off-sheet regroup
- interior regroup beyond the admitted edge window
