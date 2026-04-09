# Computational Substrate Formula-Group Listener-Anchor Live Support Evidence

Status: frozen evidence note for the FormulaGroup listener-anchor live-support closeout

## Runtime Proof

The live wiring path now proves:

- `FormulaGroup` listener anchors no longer reject out of contract on the
  already-admitted shared-group live surface
- `HostUnknown` listener anchors still reject with
  `listener_anchor_out_of_contract`

Coverage:

- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Exact Admitted-Slice Proof

The existing admitted shared-group same-text-preserve path still closes
exactly with live `FormulaGroup` listener anchors present:

- lifecycle remains exact
- mutation entry remains exact

Coverage:

- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Blocker-Removal Proof

The bounded named-range-combined preserve rerun now shows:

- lifecycle no longer fails on listener-anchor kind and instead stops at
  `opaque_dependency_surface`
- mutation entry no longer fails on listener-anchor kind and instead stops
  at `rollback_queue_or_state_mismatch`

Coverage:

- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Standalone Restore Proof

The standalone shared-group restore buckets now show a smaller, clearer
boundary:

- object realization applies with live `FormulaGroup` anchors and no
  out-of-contract reject, but still closes as `computational_mismatch`
- rollback applies with live `FormulaGroup` anchors and no out-of-contract
  reject, but still closes as `missing_restored_objects`

Coverage:

- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

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

Replay remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## Closeout Result

This evidence supports a completed blocker-removal closeout.

It proves:

- true `FormulaGroup` listener-anchor replay is now live-owned on the
  already-admitted shared-group slice
- `HostUnknown` remains outside contract
- the named-range-combined preserve blocker is now smaller and more
  explicit than before
- the admitted slice itself is unchanged in this cycle
