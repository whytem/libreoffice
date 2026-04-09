# Computational Substrate Named-Range Structural Rollout Clearance Evidence

Status: complete evidence note for the named-range structural rollout
clearance pass

## Outcome Summary

This pass closes as a real admitted-slice widening.

The bounded same-sheet global single-area named-range structural lane now
applies exactly when the structural mutation shifts the target surface without
changing its cardinality.

The pass also leaves a clean retained reject boundary:

- same-sheet shrinking `DeleteRows` stays deferred with
  `structural_population_mismatch`
- off-sheet named-range structural consumers stay rejected out of contract

## Exact Standalone Evidence

The standalone structural proof now covers the bounded same-sheet global
single-area lane directly.

The key checked-in proof buckets are:

- explicit-sheet-prefix `InsertRows` exact authority candidate proof in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- exact predicted named-range rewrite on the computational shadow surface for
  that same bounded lane

The strengthened standalone proof confirms that the bounded after-state is not
synthetic guesswork. The structural predictor and authority candidate now
close on the same surface the live tests exercise.

## Exact Live Evidence

The live narrow-rollout proof now closes exactly for representative same-sheet
global single-area shift-only cases:

- `InsertColumns`
- `InsertRows` with explicit sheet prefix
- `DeleteRows` when the target shifts but does not shrink
- `DeleteColumns`

Those proof buckets are checked in under
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx).

Each admitted bucket now asserts:

- exact applied structural result
- exact queue comparison
- exact computational comparison
- exact graph comparison
- exact IR comparison

## Runtime Seam That Was Fixed

Before the closeout patch, the bounded same-sheet named-range structural lane
was already exact on:

- cell population
- formula tree
- formula track
- group identity
- named-range identity
- IR

The remaining live mismatch was broadcaster and graph shape.

The runtime fix in
[StructuralPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx)
keeps the predicted structural after-state but uses the observed-after
broadcaster surface when materializing the bounded named-range structural
computational and graph after-state. That is the only production seam this
pass needed to touch.

## Retained Reject Evidence

The pass also freezes the remaining named-range structural boundary with live
proof:

- same-sheet `DeleteRows` that shrinks the named-range target stays deferred
  with `RejectedOutOfContract` and `structural_population_mismatch`
- off-sheet consumer cases stay rejected out of contract

That means the closeout did not blur the boundary in order to widen it.
Shift-only cases are admitted; resize and off-sheet cases remain explicit
rejects.

## Validation

The closeout validation passed:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Replay remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
