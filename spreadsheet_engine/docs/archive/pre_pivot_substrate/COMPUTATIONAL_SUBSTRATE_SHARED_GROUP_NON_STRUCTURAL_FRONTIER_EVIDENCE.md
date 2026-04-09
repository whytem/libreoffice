# Computational Substrate Shared-Group Non-Structural Frontier Evidence

Status: frozen evidence for the broader non-structural shared-group frontier closeout

## Positive Evidence

The following proof buckets now pass:

- facade-side same-text preserve classification on the shared-group
  consumer surface
- standalone exact lifecycle proof for same-text preserve in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- live narrow-rollout exact lifecycle proof for same-text preserve in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- live mutation-entry exact proof for same-text preserve in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Negative Boundary Evidence

The following broader frontier shapes remain outside the admitted slice and
are now recorded as such:

- regroup lifecycle candidate rejects
- merge lifecycle candidate rejects
- merge mutation-entry candidate rejects
- named-range plus off-sheet authority candidate rejects
- off-sheet consumer authority candidate rejects

These cases therefore do not rely on accidental normalized-equivalent live
success to justify admission.

## Validation Commands

The closeout validation set is:

- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_computational_substrate_tests -j4`
- `./spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `make -j4 CppunitTest_sc_ucalc_workbook_facade`
- `make -j4 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j4 CppunitTest_sc_ucalc_compile_diff`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_workbook_facade_tests spreadsheetengine_computational_graph_tests -j4`
- `./spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `./spreadsheet_engine/build_check/spreadsheetengine_computational_graph_tests`
- `./spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Replay Baseline

The promoted replay baseline remains unchanged:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
