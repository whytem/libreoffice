# Computational Substrate Raw Document Mutation API Evidence

Status: frozen raw-document-mutation evidence

## Purpose

This note records the bounded proof results for
[COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md)
after the contract, schema, observation, and implementation phases landed.

## Exact Admitted-Slice Result

On the admitted slice, the engine-authored primitive document-mutation path
now closes exactly in the covered proof lanes:

- the admitted raw document mutation record is present on applied mutation
  entry results
- the raw document mutation record exactly matches the admitted raw-mutation
  identity already produced upstream
- the admitted raw document mutation observation is exact on the covered
  applied lanes
- dirty-baseline rejection still records the primitive mutation shell
  explicitly before rollback
- downstream object-realization, rollback, and live-apply exactness remain
  intact

The new primitive layer therefore stayed bounded. It did not destabilize the
already-settled resident storage, resident wiring, formula-cell lifetime,
realization, rollback, or live-apply surfaces.

## Differential Findings

The differential observation pass and runtime lanes now support these
explicit statements:

- primitive document mutation execution has its own value-semantic record
  and apply verdict
- primitive mutation exactness is observable independently from the broader
  raw-mutation identity layer
- no new queue, computational, graph, or replay drift appeared on the
  admitted slice
- no new repair-detected or dirty-baseline regressions appeared in the
  covered lanes

No new deferred sub-class was discovered inside the already-admitted scalar
and narrow structural surface.

## Meaningful Host-Shell Reduction

The host-owned raw document mutation shell did shrink in a meaningful but
bounded way.

What is now explicit and engine-authored before live execution:

- primitive document-mutation identity
- primitive document-mutation record construction
- primitive document-mutation apply verdict surface
- primitive document-mutation exactness observation

What still remains host-owned in Calc:

- the primitive realization host operations around admitted execution
- the primitive rollback host operations around admitted execution
- the final verification host shell around those primitive operations

So this cycle did not remove Calc from primitive execution entirely, but it
did remove another layer of hidden host mutation intent from the admitted
slice.

## Validation Lanes

The following validation contract stayed green:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Timed Sample

The bounded timed sample for the heaviest Calc-side proof lane was:

- `CppunitTest_sc_ucalc_dependency_shadow`
- elapsed: `32.02s`
- max RSS: `1993244 KB`

This note treats that as an observation sample, not as a hard regression
budget.

## Replay Baseline

The standing replay baseline remains exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
