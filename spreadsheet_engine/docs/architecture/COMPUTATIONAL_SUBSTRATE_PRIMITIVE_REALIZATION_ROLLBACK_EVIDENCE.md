# Computational Substrate Primitive Realization And Rollback Evidence

Status: frozen primitive-realization-rollback evidence

## Purpose

This note records the bounded proof results for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md)
after the contract, schema, observation, and implementation phases landed.

## Exact Admitted-Slice Result

On the admitted slice, the engine-authored primitive realization and
primitive rollback path now closes exactly in the covered proof lanes:

- the admitted primitive realization record is present on applied
  mutation-entry results
- the admitted primitive realization observation is exact on the covered
  applied lanes
- the admitted primitive rollback record is present on dirty-baseline
  rollback results
- the admitted primitive rollback observation is exact on the covered
  rollback lanes
- downstream object-realization, rollback, raw-mutation, raw-document-
  mutation, and live-apply exactness remain intact

The new primitive layer therefore stayed bounded. It did not destabilize the
already-settled resident storage, resident wiring, formula-cell lifetime,
raw document mutation, or live-apply surfaces.

## Differential Findings

The differential observation pass and runtime lanes now support these
explicit statements:

- primitive realization execution has its own value-semantic record and
  apply verdict
- primitive rollback execution has its own value-semantic record and apply
  verdict
- primitive realization exactness is observable independently from the
  broader object-realization identity layer
- primitive rollback exactness is observable independently from the broader
  rollback identity layer
- no new queue, computational, graph, or replay drift appeared on the
  admitted slice
- no new repair-detected or dirty-baseline regressions appeared in the
  covered lanes

No new deferred sub-class was discovered inside the already-admitted scalar
and narrow structural surface.

## Meaningful Host-Shell Reduction

The host-owned primitive realization and rollback shell did shrink in a
meaningful but bounded way.

What is now explicit and engine-authored before final verification:

- primitive realization identity
- primitive realization record construction
- primitive realization apply verdict surface
- primitive realization exactness observation
- primitive rollback identity
- primitive rollback record construction
- primitive rollback apply verdict surface
- primitive rollback exactness observation

What still remains host-owned in Calc:

- the final verification host shell around admitted primitive realization
  and rollback execution

So this cycle did not remove Calc from final host execution entirely, but it
did remove another layer of hidden realization and rollback intent from the
admitted slice.

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
- elapsed: `6.92s`
- max RSS: `248692 KB`

This note treats that as an observation sample, not as a hard regression
budget.

## Replay Baseline

The standing replay baseline remains exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
