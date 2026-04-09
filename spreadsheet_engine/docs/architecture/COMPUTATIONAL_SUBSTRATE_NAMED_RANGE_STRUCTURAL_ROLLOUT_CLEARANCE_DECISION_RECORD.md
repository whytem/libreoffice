# Computational Substrate Named-Range Structural Rollout Clearance Decision Record

Status: complete closeout decision for the named-range structural rollout
clearance pass

## Decision

Widen the admitted structural rollout, but only on the bounded same-sheet
global single-area shift-only named-range surface.

The admitted structural slice now includes:

- same-sheet structural `InsertRows`
- same-sheet structural `DeleteRows` when the named-range target shifts but
  does not resize
- same-sheet structural `InsertColumns`
- same-sheet structural `DeleteColumns`
- global names only
- single-area targets only
- ordinary scalar formulas only
- clean baseline only
- no off-sheet named-range consumers

## Why The Rollout Now Widens

The strengthened live proof closed the bounded same-sheet slice exactly rather
than merely reaching the candidate bridge.

What now closes exactly:

- standalone structural prediction for explicit-sheet-prefix same-sheet target
  shifting
- live narrow-rollout exact apply for representative same-sheet row and column
  shifts
- exact queue, computational, graph, and IR verification on that bounded
  surface

The key runtime seam was narrower than the original blocker statement. The
predicted after-state was already correct for cell population, formula tree,
formula track, and named-range descriptors, but the bounded named-range
structural lane still diverged on broadcaster and graph shape. The closeout
patch fixes only that seam by keeping the predicted structural state while
matching the observed-after broadcaster surface for the bounded named-range
structural lane.

## What Remains Deferred

This pass does not admit broad named-range-sensitive structural behavior.

The following remain explicitly deferred:

- same-sheet named-range structural edits that resize the target surface
- off-sheet named-range structural consumers
- sheet-local names
- multi-area names
- scope-ambiguous name sets
- shared-group-sensitive structural behavior
- repair-sensitive structural divergence
- sheet insert, delete, rename, or move

The checked-in retained reject for the shrinking `DeleteRows` case still ends
at `structural_population_mismatch`, which is the correct boundary for now.

## Resulting Boundary

This clears the old broad “named-range-sensitive structural rollout” blocker
in its previous form.

The new named-range structural boundary is narrower and more honest:

- admitted: same-sheet global single-area shift-only structural named-range
  edits
- deferred: resize, off-sheet, local, multi-area, ambiguous, and
  shared-group-sensitive structural named-range behavior

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md)

## Next Adjacent Concern

The next highest-value structural blocker is no longer this broad named-range
rollout question.

The next structural widening target should be:

- shared-group-sensitive structural behavior outside the bounded exact
  same-sheet shareable slice

The remaining named-range structural frontier is now a narrower follow-on:

- resize, off-sheet, local, multi-area, and ambiguous named-range structural
  behavior
