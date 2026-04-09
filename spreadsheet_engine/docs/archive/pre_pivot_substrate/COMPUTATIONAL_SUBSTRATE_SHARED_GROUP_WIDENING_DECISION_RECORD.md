# Computational Substrate Shared-Group Widening Decision Record

Status: complete closeout decision for the shared-group widening plan

## Decision

Widen the live opt-in rollout, but only on one bounded shared-group
structural family.

The bounded shared-group widening cycle closes with this narrower result:

- admit exact same-sheet shareable structural shared-group `Preserve`,
  `Split`, and `Rebuild` cases on the already-admitted mutation vocabulary
- keep non-exact shared-group behavior on the validation-only lane
- keep the dedicated shared-group gate as the explicit promotion boundary for
  this slice

## Why The Structural Family Now Admits

The cycle now establishes the exact condition that was still partial after
the earlier preserve-first widening step:

- exact engine-predicted ownership of after-mutation shared-group topology on
  the bounded structural slice
- exact live apply on that slice through queue, computational, graph, and IR
  verification
- a materializable after-state that keeps live observed cell and formula
  payloads by address while preserving engine-authored group identity

The key boundary shift is specific:

- the admitted structural path now predicts after-topology by shifting
  surviving shared-group members and partitioning them into contiguous runs
- that exact rule family naturally covers preserve, split, and rebuild
  outcomes
- the host-observed topology overlay remains only as a validation fallback
  for non-exact cases

That is enough for one bounded promotion into the live rollout.

## What The Cycle Still Does Not Establish

The cycle still does not establish live admission for broader shared-group
behavior.

What remains unsettled:

- non-structural shared-group split or rebuild outcomes from scalar, formula,
  or clear mutations
- exact live authority for regroup or merge outcomes across prior groups
- repair-sensitive shared-group divergence beyond repair-detected rollback
- shared-group behavior combined with named-range-sensitive structure
- off-sheet or broader workbook classes outside the bounded same-sheet slice

Those classes do not yet have the same engine-predicted authority closure as
the admitted structural family.

## Final Boundary

The live admitted rollout therefore becomes:

- admitted scalar lifecycle authority
- admitted scalar mutation entry
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- ordinary scalar formulas plus exact same-sheet shareable shared-group
  structural `Preserve`, `Split`, and `Rebuild` cases
- clean baseline only
- no named-range-sensitive structural behavior

The widened shared-group structural family is still explicitly bounded by:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL=1`
- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP=1`

## Classes Left Deferred

The following remain explicitly deferred:

- non-exact shared-group split, rebuild, regroup, and broader repair-sensitive
  live rollout
- non-structural shared-group split or rebuild behavior
- shared-group behavior combined with named-range-sensitive structure
- host-only regrouping or repair that is not representable through the
  current seams
- off-sheet or broader workbook classes outside the bounded single-sheet
  structural slice
- sheet insert, delete, rename, or move
- copy, move, clipboard, load-time, or undo-like structural flows

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md)

## Next Adjacent Concern

The next adjacent concern should stay narrower than another broad
workbook-class push.

The right next question is:

- whether non-structural shared-group split or rebuild outcomes can replace
  their remaining validation-only observed-topology dependency with exact
  engine-authored state and therefore earn the same live verification
  standard as the admitted structural family

The evidence does not support jumping directly from this result into broader
shared-group repair, named-range-combined classes, or another implicit
rollout widening.
