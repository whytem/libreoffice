# Computational Substrate Final Rollback Evidence

Status: frozen final-rollback evidence

## Purpose

This note records the bounded proof results for
[COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md)
after the rollback observation and implementation workstreams closed.

The goal of this evidence pass is to answer one bounded question:

- does admitted-slice rollback now restore from an explicit engine-authored
  rollback record closely enough to justify shrinking the retained host-owned
  rollback boundary?

## Exact Restore Results

The following admitted rollback proof lanes closed exactly:

- helper-level rollback restore from an explicit
  `AdmittedRollbackRecord` on the ordinary scalar formula slice
- exact restore of resident cell storage, resident wiring, formula-cell
  lifetime realization, formula-tree state, and formula-track state in that
  helper-level rollback lane
- runtime dirty-baseline rejection in mutation entry, with rollback now
  reporting explicit `Exact` rollback observation instead of silently
  restoring through an ad hoc local path

These results mean the rollback path now has an explicit engine-authored
surface on the admitted slice rather than rebuilding before-state at the
moment rollback is needed.

## Explicit Non-Exact And Diagnostic Results

The following diagnostic lanes also behaved as intended:

- synthetic classifier coverage closed for every rollback observation kind
- removing a restored admitted formula object after rollback is classified as
  `MissingRestoredObjects`
- host-only broadcaster canonicalization patterns remain isolated in the
  rollback observation model instead of being folded into generic mismatch

These are useful because they keep rollback failures explicit and bounded
rather than letting the host shell quietly normalize them away.

## What This Proves About The Host Boundary

The host-owned rollback layer did shrink in a meaningful way:

- the admitted rollback record is now engine-authored
- mutation entry now reuses that same rollback record for dirty-baseline
  rejection, verification failure, and repair-detected rollback
- rollback observation is now attached to the runtime result, so the host no
  longer hides whether rollback restored exactly

What remains host-owned is narrower:

- Calc still executes the rollback shell
- Calc still owns raw document mutation APIs
- Calc still owns the final rollback host around out-of-contract workbook
  classes

## Performance And Replay Observations

Standing validation stayed green:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

One measured replay sample for this evidence pass was:

- elapsed: `57.01s`
- max RSS: `26272KB`

The replay baseline remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

No replay regression or fallback reintroduction was observed in this cycle.

## Evidence Limits

This evidence does not justify broader rollback ownership for:

- shared-group-sensitive rollback
- named-range-sensitive rollback
- sheet-wide structural classes
- raw mutation API migration

It only proves that the admitted scalar and narrow structural slice now has a
meaningfully smaller retained rollback boundary than it did at plan entry.
