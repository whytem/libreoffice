# Computational Substrate Wiring Container Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-resident wiring containers on the admitted slice.

Do not treat this as proof for broad `ScDocument` dependency-container
migration, formula-cell object lifetime migration, or direct mutation-entry
migration.

## What Is Now Justified

The bounded admitted slice now has checked-in proof for:

- engine-owned resident cell storage on the admitted slice
- engine-owned resident wiring containers on the admitted slice
- engine-owned mutable computational sidecar state
- engine-owned graph, wiring, and recalc target decisions
- Calc-side realization of admitted live cells from engine-owned resident
  storage
- Calc-side realization of admitted live listener, broadcaster,
  formula-tree, and formula-track state from engine-owned resident wiring
  containers with exact verification

That is strong enough to treat admitted-slice live wiring-container residency
as a real engine-owned boundary surface.

## What Is Still Not Justified

This closeout does not justify:

- broad `ScDocument` dependency-container migration
- formula-cell object lifetime migration
- direct engine-owned document mutation entry
- shared-group-sensitive lifecycle or structural migration
- named-range-sensitive structural rollout
- sheet-level structural edits
- token-container ownership migration

## Next Adjacent Concern

Because admitted-slice cell and wiring residency are now proven, the next
adjacent migration concern is:

- formula-cell object lifetime on the admitted slice

That should come before any claim of broad document-residency transfer.
Calc can continue to host mutation entry and final rollback while the project
decides whether live formula-cell lifetime can follow the now-proven resident
cell and resident wiring boundary.

## Working Conclusion

The project should now treat the admitted slice as:

- engine-owned for resident cell storage, resident wiring containers,
  mutable computational state, and graph/wiring/queue decisions
- Calc-owned for mutation entry, formula-cell object lifetime, live host
  realization, and final rollback

That is a meaningful additional boundary shift, but it remains intentionally
narrow.
