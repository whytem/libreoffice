# Computational Substrate Phase 1 Decision Record

Status: completed Phase 1 decision record

## Question

Can the engine-owned computational shadow now justify Phase 2 live dependency
graph shadow work?

## Decision

Proceed to Phase 2 on the narrowed Phase 1 subset, widened to include the
representative structural rebuild cases admitted during Phase 1.

## Why

Phase 1 produced the required artifacts:

- schema artifact in
  [COMPUTATIONAL_SUBSTRATE_PHASE1_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE1_SCHEMA.md)
- builder surface in
  [ComputationalShadowBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx)
- mapping artifact in
  [COMPUTATIONAL_SUBSTRATE_PHASE1_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE1_MAPPING_RULES.md)
- rebuild-based mutation surface in
  [ComputationalShadowMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadowMutation.hxx)
- differential comparison surface in
  [ComputationalShadowComparison.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx)
- representative structural evidence in
  [COMPUTATIONAL_SUBSTRATE_PHASE1_STRUCTURAL_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE1_STRUCTURAL_WIDENING_EVIDENCE.md)

The Phase 1 validation lanes now demonstrate:

- full shadow reconstruction from facade plus normalized live observation
- pointer-free identity and mapping rules
- deterministic rebuild after scalar, formula, clear, and named-range edits
- automated differential comparison between shadow state and live captures
- representative row-insert and column-delete rebuild survival

That is enough to justify the next phase.

## Admitted Phase 2 Entry Subset

Phase 2 may start from the following admitted substrate surface:

- non-empty computation-facing cell population
- formula descriptors and formula-group descriptors
- formula-tree and formula-track membership as observed state
- normalized broadcaster and listener anchor state
- rebuild-based shadow maintenance for:
  - `SetValue`
  - formula edit or insertion
  - `ClearCell`
  - named-range edit
- representative structural rebuilds for:
  - single row insert
  - single column delete

## Explicit Defers

Phase 1 does not justify immediate widening to:

- authoritative storage ownership
- incremental shadow updates as the required strategy
- broad structural authority across all row/column cases
- BASM slot-layout fidelity
- Calc token-container migration
- listener lifecycle authority
- copy/move, clipboard rebuild, or load-time `CalcAfterLoad` authority

Those remain later-phase questions.

## Validation Summary

The closeout validation used:

- `git diff --check`
- `make -j1 CppunitTest_sc_ucalc_workbook_facade`
- targeted computational-substrate cases in `CppunitTest_sc_ucalc_dependency_shadow`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_workbook_facade_tests spreadsheetengine_computational_substrate_tests -j4`
- `spreadsheet_engine/build_check/spreadsheetengine_workbook_facade_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

The standing replay baseline remains unchanged and green.

## Risk Gate Outcome

The Phase 1 gate is passed.

Proceed to Phase 2, but keep the initial live dependency-graph shadow work
anchored to the admitted subset above rather than assuming broad substrate
authority from day one.
