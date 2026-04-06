# Computational Substrate Shared-Group Non-Structural Evidence

Status: complete evidence note for the non-structural shared-group admission cycle

## Purpose

This note records the checked-in proof outcomes for the bounded
non-structural shared-group promotion cycle.

Its job is to say what now admits, what stays exact, and what still remains
outside the live slice.

## Evidence Summary

The current proof surface separates into five classes:

- exact facade-side classification for representative non-structural split
  and rebuild outcomes
- one exact same-sheet shareable `SetScalarValue` authority lane in the
  admitted slice
- one exact same-sheet shareable `SetFormula` lifecycle lane in the admitted
  slice
- one exact bounded mutation-entry lane on the same authority/lifecycle
  family
- deterministic gate-off rejection and standing zero-fallback replay

That is now live-admission evidence for one bounded non-structural
shared-group family, but not for broad regrouping shared-group behavior.

## Exact Classification Evidence

The checked-in Calc-side classification coverage now proves that:

- scalar overwrite of a shared-group member can be classified explicitly as
  `Split`
- formula replacement on a shared-group anchor can be classified explicitly
  as `Rebuild`

That keeps the non-structural cycle on the same explicit transition surface
already used by the structural shared-group closeout.

## Exact Authority And Lifecycle Evidence

The key positive runtime proof buckets are:

- same-sheet shareable member-exit `SetScalarValue` on a shared-group anchor
- same-sheet shareable member-exit `SetFormula` on a shared-group member

Those buckets now establish that:

- the predictor authors after-group topology by removing the touched member
  and repartitioning surviving members into contiguous runs
- the predictor rejects when observed-after shared-group topology does not
  exactly match the engine-authored result
- the authority or lifecycle candidate closes exact queue, computational,
  graph, and IR verification
- the live narrow-rollout path leaves the expected after-group topology in
  the Calc document

## Exact Mutation-Entry Evidence

The bounded non-structural slice now also closes through mutation entry.

The checked-in positive mutation-entry buckets are:

- same-sheet shareable shared-group `SetScalarValue`
- same-sheet shareable shared-group `SetFormula`

Those buckets now prove that:

- the mutation-entry path routes the bounded non-structural slice through
  authority or lifecycle as appropriate
- admitted object realization no longer rejects non-matrix shared formulas
  already present in the live document
- final exact computational and graph verification still closes on the
  admitted after-state

## Reject And Deferred Evidence

The cycle keeps deterministic negative-path coverage for the explicit
boundary:

- gate-off shared-group non-structural authority entry rejects out of
  contract
- same-text preserve, regroup, merge, named-range-combined, repair-sensitive,
  and off-sheet classes remain outside the admitted slice

That keeps the widened boundary explicit instead of inferred.

## Exact Closure And Remaining Limits

The current evidence now shows one meaningful non-structural shared-group
subset that meets the live-admission standard:

- same-sheet shareable member-exit `SetScalarValue`, `SetFormula`, and
  `ClearCell` cases where the touched cell exits the group and surviving
  members are repartitioned into contiguous runs

The current evidence does not show live-admission closure for:

- same-text preserve replacements that intentionally keep the group
- regroup or merge behavior across prior groups
- named-range-combined shared-group behavior
- repair-sensitive host-only normalization
- off-sheet or broader workbook classes

## Standing Validation

The closeout evidence above is backed by:

- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

The standing replay baseline remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
