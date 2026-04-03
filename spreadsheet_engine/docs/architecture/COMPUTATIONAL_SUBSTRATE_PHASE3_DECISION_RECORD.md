# Computational Substrate Phase 3 Decision Record

Status: complete closeout decision for Phase 3

## Decision

Proceed to Phase 4, but only on the narrowed IR-backed subset actually
validated in Phase 3.

Phase 3 proved that `spreadsheet_engine/` can hold an engine-owned
execution-facing IR shadow for the admitted computational-substrate subset
without treating `ScTokenArray` pointer identity, pool placement, or Calc
container order as the durable authority model.

This is not yet live execution authority. It is a validated IR boundary with:

- explicit schema and lowering rules
- explicit reference-update and representative structural-update semantics
- explicit exact-or-normalized differential comparison rules
- explicit build-failure recording instead of hidden token fallbacks

## What Phase 3 Now Admits

The admitted Phase 3 IR subset is:

- formula-cell execution IR records keyed by engine-owned shadow cell ids
- formula-group membership carried into the IR shadow where the admitted subset
  already exposes it
- deterministic lowering from admitted compiled-formula output into:
  - scalar literals
  - scalar references
  - range references
  - named references
  - external reference shapes
  - table, database, matrix, jump, whitespace, and error carriers where the
    admitted compiler output already provides them
- explicit lowering of the representative admitted execution cases validated
  in Phase 3:
  - simple arithmetic and scalar references
  - range and named-range calls
  - admitted lexical jump-form lowering
- IR reference-update and representative structural-adjustment behavior for:
  - scalar references
  - range references
  - column-row-name reference shape
  - external single and range references
  - single row insert
  - single column delete
- rebuild-based IR differential validation for:
  - `SetValue`
  - formula edit
  - formula insertion
  - `ClearCell`
  - named-range rename
  - representative row insert
  - representative column delete
- exact or intentionally normalized IR comparison verdicts:
  - `Exact`
  - `NormalizedEquivalent`
  - `Mismatch`

The accepted comparison contract is:

- `Exact` where snapshot, grammar, formula records, formula groups, and build
  failures already match directly
- `NormalizedEquivalent` where the same IR meaning is present but formula/group
  ordering differs only in the explicitly normalized ways
- never `Mismatch` on the admitted subset

## What Remains Deferred

Phase 3 does not admit:

- live execution authority for the IR shadow
- replacement of Calc execution with direct IR execution
- broad `ScTokenArray` migration or removal from Calc compilation/runtime
- listener, broadcaster, or graph authority transfer
- formula lifecycle authority
- copy, move, clipboard, or load-time IR authority
- broad structural-edit authority beyond the admitted representative cases
- external-reference cache ownership or other host-heavy service migration

These remain Phase 4-or-later questions, and only if the next phase stays on
the narrowed pilot surface actually validated here.

## Evidence

The checked-in Phase 3 artifacts are:

- [COMPUTATIONAL_SUBSTRATE_PHASE3_IR_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE3_IR_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE3_IR_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE3_IR_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE3_REFERENCE_UPDATE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE3_REFERENCE_UPDATE_EVIDENCE.md)
- [ExecutionIrBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIrBuilder.hxx)
- [ExecutionIrComparison.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx)
- [ExecutionIrMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIrMutation.hxx)

## Validation Summary

Phase 3 closeout validation is green on:

- `CppunitTest_sc_ucalc_workbook_facade`
- targeted IR and graph cases in `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Standing replay baseline remains:

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate
