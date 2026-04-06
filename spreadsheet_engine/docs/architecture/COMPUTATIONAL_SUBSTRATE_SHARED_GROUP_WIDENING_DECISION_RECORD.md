# Computational Substrate Shared-Group Widening Decision Record

Status: complete closeout decision for the shared-group widening plan

## Decision

Do not widen the live opt-in rollout.

The bounded shared-group widening cycle closes with a narrower result:

- keep shared-group-sensitive structural behavior out of the live rollout
- keep the bounded same-sheet shared-group lane as validation-only
- keep the dedicated shared-group gate as a proof surface rather than as a
  live promotion gate

## Why The Cycle Stays Validation-Only

The evidence is strong enough to justify one bounded result, but not a live
authority expansion.

What the cycle did establish:

- the workbook facade now exposes explicit shared-group transition
  classification for preserve, rebuild, split, and none
- same-shape `SetFormula` preserve and `ClearCell` split outcomes are now
  checked in as explicit proof buckets
- same-sheet shareable structural preserve cases can enter the structural
  pilot when the dedicated shared-group gate is enabled
- gate-off and named-range-combined classes reject deterministically
- repair-sensitive divergence is still repair-detected and rollback-capable

What the cycle did not establish:

- exact engine-predicted ownership of after-mutation shared-group topology
- an exact live apply on any shared-group-containing slice
- a proof surface that no longer depends on host-observed after-state group
  topology

That last point is the one bounded reason this cycle cannot promote into the
live rollout:

- the current pilot still uses host-observed after-state shared-group
  topology for group identity comparison

So the cycle closes as validation-only rather than as live admission.

## Final Boundary

The live admitted rollout therefore remains:

- admitted scalar lifecycle authority
- admitted scalar mutation entry
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- ordinary scalar formulas only
- clean baseline only
- no shared groups
- no named-range-sensitive structural behavior

The only new settled shared-group conclusion is narrower than rollout
admission:

- a bounded same-sheet shareable shared-group class now has explicit facade
  classification and a dedicated validation-only structural pilot lane

## Classes Left Deferred

The following remain explicitly deferred:

- all live shared-group structural rollout
- shared-group behavior combined with named-range-sensitive structure
- host-only regrouping or repair that is not representable through the
  current seams
- off-sheet or broader workbook classes outside the bounded single-sheet
  pilot
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

The next adjacent concern should be narrower than another broad workbook-class
push.

The right next question is:

- whether the same-sheet shareable preserve slice can replace
  host-observed after-state shared-group topology with exact engine-predicted
  topology and therefore meet the existing live verification standard

The evidence does not support jumping directly from this result into broader
shared-group repair, named-range-combined classes, or another implicit
rollout widening.
