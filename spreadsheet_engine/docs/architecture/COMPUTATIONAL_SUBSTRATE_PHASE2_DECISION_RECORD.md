# Computational Substrate Phase 2 Decision Record

Status: complete closeout decision for Phase 2

## Decision

Proceed to Phase 3, but only on the narrowed graph-shadow subset actually
validated in Phase 2.

Phase 2 proved that `spreadsheet_engine/` can hold an engine-owned live
dependency graph shadow for the admitted Phase 1 subset without treating Calc
listener or broadcaster containers as the durable identity model.

This is not yet graph authority. It is a validated shadow lane with explicit
normalization rules and explicit defer boundaries.

## What Phase 2 Now Admits

The admitted Phase 2 graph subset is:

- formula-cell graph nodes
- formula-group graph nodes
- normalized listener-anchor nodes
- cell broadcaster nodes
- area broadcaster nodes
- broadcaster-to-listener edge sets
- graph-facing formula-tree and formula-track subsets
- rebuild-based graph maintenance for:
  - `SetValue`
  - formula edit
  - formula insertion
  - `ClearCell`
  - named-range rename in the standalone differential lane
- explicit delayed-state comparison for:
  - delayed listener startup
  - delayed broadcaster deletion
- representative structural rebuild coverage for:
  - single row insert
  - single column delete

The accepted comparison contract is:

- `Exact` where raw captured subset order and graph subset order already agree
- `NormalizedEquivalent` where semantic node and edge identity agree but the
  raw tree/track order is intentionally normalized by the graph shadow
- never `Mismatch` on the admitted subset

## What Remains Deferred

Phase 2 does not admit:

- graph authority or listener/broadcaster ownership transfer
- BASM slot layout or broadcaster-container migration
- Calc listener-context migration
- `ScDocument`, `ScTable`, or `ScColumn` storage authority
- incremental graph maintenance as a required strategy
- copy / move / clipboard graph authority
- load-time `CalcAfterLoad` graph authority
- `ScTokenArray` migration or execution-IR authority

These remain Phase 3-or-later questions, and only if the next phase is scoped
to the narrowed subset validated here.

## Evidence

The checked-in Phase 2 artifacts are:

- [COMPUTATIONAL_SUBSTRATE_PHASE2_GRAPH_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE2_GRAPH_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE2_GRAPH_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE2_GRAPH_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE2_SPECIAL_CASE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE2_SPECIAL_CASE_EVIDENCE.md)

## Validation Summary

Phase 2 closeout validation is green on:

- `CppunitTest_sc_ucalc_workbook_facade`
- targeted graph-shadow cases in `CppunitTest_sc_ucalc_dependency_shadow`
- `spreadsheetengine_computational_graph_tests`
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
