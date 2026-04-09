# Computational Substrate Off-Sheet Dependency Closure Evidence

Status: completed evidence for the bounded off-sheet widening pass

## Admitted Result

This pass admitted the following new bounded off-sheet family:

- exact same-workbook one-consumer-sheet direct off-sheet shared-group
  non-structural `MemberExit` for `SetScalarValue`, `SetFormula`, and
  `ClearCell`

The pass did not admit broader off-sheet behavior.

## Unlocking Change

The key blocker removal is in
[../../inc/spreadsheetengine/detail/dependency/DependencySnapshot.hxx](../../inc/spreadsheetengine/detail/dependency/DependencySnapshot.hxx).

The dependency snapshot now falls back to raw-reference parsing when a
formula-parser `NamedReference` token does not resolve to an actual named
range. That closes the bounded cross-sheet `Data.B1` / `Data.B2` / `Data.B3`
surface instead of classifying it as `missing_named_reference` and turning
the whole candidate into `opaque_dependency_surface`.

## Standalone Proof

Engine-only exactness proof is covered in:

- [../../tests/unit/computational_substrate_tests.cxx](../../tests/unit/computational_substrate_tests.cxx)

That proof now demonstrates:

- applicable authority planning for bounded off-sheet shared-group
  `MemberExit`
- exact computational after-state
- exact graph after-state
- exact IR after-state

## Live Calc Proof

Live Calc proof is covered in:

- [../../../sc/qa/unit/ucalc_dependency_shadow.cxx](../../../sc/qa/unit/ucalc_dependency_shadow.cxx)

Admitted live buckets now include:

- authority off-sheet member-exit `SetScalarValue`
- authority off-sheet member-exit `SetFormula`
- authority off-sheet member-exit `ClearCell`
- lifecycle off-sheet member-exit `SetFormula`
- lifecycle off-sheet member-exit `ClearCell`
- mutation-entry off-sheet member-exit `SetScalarValue`
- mutation-entry off-sheet member-exit `SetFormula`
- mutation-entry off-sheet member-exit `ClearCell`

These proofs also freeze an important host-shape detail:

- under authority and lifecycle with `AutoCalc` disabled, the off-sheet
  consumer closes exactly on queue/graph/verification without requiring
  immediate eager cached-value replacement on the dependent cross-sheet
  formula
- mutation entry closes the full admitted path on the same bounded family

## Retained Reject And Defer Proof

The pass intentionally retains:

- off-sheet named-range-combined boundary as deferred in
  [../../../sc/qa/unit/ucalc_workbook_facade.cxx](../../../sc/qa/unit/ucalc_workbook_facade.cxx)
- structural off-sheet global named-range rejection in
  [../../../sc/qa/unit/ucalc_dependency_shadow.cxx](../../../sc/qa/unit/ucalc_dependency_shadow.cxx)

## Validation

Validated in this pass:

- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_computational_substrate_tests`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Replay remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
