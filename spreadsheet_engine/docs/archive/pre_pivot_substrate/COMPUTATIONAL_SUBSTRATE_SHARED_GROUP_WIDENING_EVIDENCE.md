# Computational Substrate Shared-Group Widening Evidence

Status: complete evidence note for the shared-group widening plan

## Purpose

This note records the checked-in proof outcomes for the bounded shared-group
widening cycle.

Its job is to say what now admits, what stays stable, and what still keeps
broader shared-group-containing workbooks out of the live opt-in rollout.

## Evidence Summary

The current proof surface separates into six classes:

- exact facade-side transition classification for preserve, split, and
  rebuild outcomes
- one exact same-sheet shareable structural preserve class in the admitted
  lane
- one exact same-sheet shareable structural split class in the admitted lane
- one exact same-sheet shareable structural rebuild class in the admitted
  lane
- deterministic reject and defer classes that stay out of contract
- one repair-detected structural divergence class that still rolls back

That is now live-admission evidence for one bounded shared-group structural
family, but not for broad shared-group behavior.

## Exact Classification Evidence

The first strong result in this cycle is the explicit shared-group
classification surface now exposed through the workbook facade.

The checked-in Calc-side coverage proves that:

- same-shape `SetFormula` replacement on a shared-group member can be
  classified explicitly as `Preserve`
- `ClearCell` on a shared-group member can be classified explicitly as
  `Split`
- row-structural shared-group mutations can now be classified explicitly as
  `Preserve`, `Split`, or `Rebuild`

This matters because the widening cycle no longer depends on hidden Calc-local
interpretation just to describe what happened to a group.

## Exact Structural Authority Evidence

The key new result is that one bounded family of structural shared-group
cases now closes exact authority-style verification.

The checked-in positive proof buckets are:

- same-sheet shareable `Preserve` after shifted structure
- same-sheet shareable `Split` after inserting through a two-cell group
- same-sheet shareable `Rebuild` after inserting through a longer group

Those candidates now prove all of the following:

- the structural predictor carries after-topology forward by shifting
  surviving members and partitioning them into contiguous runs
- the predictor still succeeds when observed after-topology is deliberately
  corrupted and the authority-path fallback is disabled
- the authority candidate closes exact queue, computational, graph, and IR
  state
- the live Calc narrow-rollout path applies the candidate and leaves the
  expected after-group topology in the document

That is the bounded live-admission result this cycle was aiming for.

## Reject And Deferred Evidence

The widening cycle still has deterministic negative-path coverage for the
important deferred classes:

- gate-off shared-group structural candidate entry rejects out-of-contract
- shared-group behavior combined with named-range-sensitive structure rejects
  out-of-contract even with the dedicated shared-group gate enabled

That keeps the widened boundary explicit instead of inferred.

## Rollback And Repair Evidence

The widening cycle also preserved the structural safety model outside the
admitted exact-topology slice.

The checked-in repair-sensitive case shows that:

- deliberate post-edit perturbation of the shared-group shape returns
  `RepairDetected`
- the structural path rolls back rather than accepting divergent shared-group
  topology

This keeps the boundary honest around host repair and regrouping behavior.

## Exact Closure And Remaining Limits

The current evidence now shows one meaningful shared-group subset that meets
the live-admission standard for exact engine-owned authority:

- exact same-sheet shareable structural shared-group topology cases predicted
  by shifting surviving members and partitioning them into contiguous runs

The current evidence does not show live-admission closure for:

- non-structural shared-group split or rebuild outcomes from scalar, formula,
  or clear mutations
- shared-group regrouping that merges distinct prior groups
- broader repair-sensitive shared-group behavior
- named-range-combined shared-group structure
- off-sheet or broader workbook classes

The bounded remaining reason is explicit:

- the observed-topology overlay still exists for non-exact validation cases

So the cycle established one admitted structural family, not broad
shared-group rollout.

## Memory And Performance Observations

No material regression signal was observed in the standing closeout checks.

The bounded observations are:

- the widened shared-group structural family is still behind dedicated
  structural and shared-group gates
- the observed shared-group topology overlay is now limited to the
  validation-only fallback path
- the replay baseline remained exact with zero cached fallback

That last point is the strongest standing operational signal:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## What This Evidence Supports

This evidence supports only the following closeout conclusion:

- widen the live rollout on exact same-sheet shareable structural shared-group
  preserve, split, and rebuild cases
- keep non-exact, non-structural, named-range-combined, repair-sensitive,
  and broader shared-group classes outside the admitted slice

It does not support broad live rollout admission of shared-group-containing
workbooks.

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
