# Computational Substrate Shared-Group Widening Evidence

Status: complete evidence note for the shared-group widening plan

## Purpose

This note records the checked-in proof outcomes for the bounded shared-group
widening cycle.

Its job is to say what was observed, what stayed stable, and what still keeps
shared-group-containing workbooks out of the live opt-in rollout.

## Evidence Summary

The current proof surface separates into five classes:

- exact facade-side transition classification for preserve and split outcomes
- one same-sheet shareable structural preserve class that reaches the
  validation-only pilot
- deterministic reject and defer classes that stay out of contract
- one repair-detected structural divergence class that rolls back
- no live-admission class that proves exact engine-owned shared-group
  authority

That is useful closeout evidence, but it is still not live-admission
evidence.

## Exact Classification Evidence

The strongest positive result in this cycle is the explicit shared-group
classification surface now exposed through the workbook facade.

The checked-in Calc-side coverage proves that:

- same-shape `SetFormula` replacement on a shared-group member can still be
  classified explicitly as `Preserve`
- `ClearCell` on a shared-group member is classified explicitly as `Split`
- shifted row-structural preserve outcomes can be described through the same
  descriptor-based transition lane

This matters because the widening cycle no longer depends on hidden Calc-local
interpretation just to describe what happened to a group.

## Validation-Only Structural Evidence

Calc differential evidence now shows that one bounded shared-group class can
enter the structural pilot when both structural gates are enabled:

- same-sheet only
- shareable shared-group topology
- carried-forward structural mutation vocabulary
- no named ranges
- clean baseline

The checked-in preserve case is:

- single-sheet `InsertRows`
- shared-group preserve after address shifting
- contract class `ValidationOnly`

This is a real pilot lane, but it is still explicitly validation-only.

## Reject And Deferred Evidence

The widening cycle now has deterministic negative-path coverage for two
important classes:

- gate-off shared-group structural candidate entry rejects out-of-contract
- shared-group behavior combined with named-range-sensitive structure rejects
  out-of-contract even with the dedicated shared-group gate enabled

That is the right result for this cycle.

It means the shared-group pilot did not widen the live slice by implication,
and the named-range-combined class stayed explicitly deferred rather than
being silently absorbed into a broader pilot.

## Rollback And Repair Evidence

The widening cycle also preserved the existing structural safety model.

The checked-in repair-sensitive case shows that:

- deliberate post-edit perturbation of the shared-group shape returns
  `RepairDetected`
- the structural path rolls back rather than accepting divergent shared-group
  topology

This keeps the pilot honest around host repair and regrouping behavior.

## Exact Closure And Live Admission

The current evidence does not show a meaningful shared-group subset that now
meets the live-admission standard for exact engine-owned authority.

The bounded reason is explicit:

- the validation-only pilot still uses host-observed after-state shared-group
  topology for group identity comparison

That means the cycle can classify preserve, split, rebuild, repair, and defer
outcomes honestly, but it does not yet prove that the engine can own the
after-mutation shared-group topology itself on the live path.

So the cycle did not establish exact live closure for:

- resident cell state
- resident wiring state
- lifetime and realization
- rollback and final verification

on any shared-group-containing live-admission slice.

## Memory And Performance Observations

No material regression signal was observed in the standing closeout checks.

The bounded observations are:

- the shared-group pilot is behind dedicated structural and shared-group
  gates
- the extra shared-group topology overlay is used only inside the
  validation-only pilot path
- the replay baseline remained exact with zero cached fallback

That last point is the strongest standing operational signal:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## What This Evidence Supports

This evidence supports only the following closeout conclusion:

- keep the live rollout unchanged
- keep the bounded shared-group structural lane as validation-only
- keep named-range-combined, repair-sensitive, and broader shared-group
  classes explicitly deferred

It does not support live rollout admission of shared-group-containing
workbooks yet.

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
