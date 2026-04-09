# Computational Substrate Cell Storage Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-resident cell storage on the admitted slice.

Do not treat this as proof for broad `ScDocument` storage migration, broad
formula-cell lifetime migration, or live broadcaster/listener container
residency migration.

## What Is Now Justified

The bounded admitted slice now has checked-in proof for:

- engine-owned resident cell storage on the admitted slice
- engine-owned mutable computational sidecar state
- engine-owned graph and wiring target sets
- Calc-side mirroring of admitted live cell state from the engine-owned
  resident store
- Calc-side replay of admitted live listener, broadcaster, formula-tree, and
  formula-track state from engine-owned targets with exact verification

That is strong enough to treat admitted-slice physical cell residency as a
real engine-owned boundary surface.

## What Is Still Not Justified

This closeout does not justify:

- broad `ScDocument` storage migration
- formula-cell object lifetime migration
- broad broadcaster/listener container residency migration
- shared-group-sensitive lifecycle or structural migration
- named-range-sensitive structural rollout
- sheet-level structural edits
- token-container ownership migration

## Next Adjacent Concern

Because admitted-slice cell residency is now proven, the next adjacent
migration concern is:

- live broadcaster/listener container residency on the admitted slice

That should come before any claim of broad document-storage migration. Calc
can continue to host formula-cell lifetime, mutation entry, and final
rollback while the project decides whether live listener and broadcaster
container residency can follow the now-proven resident cell and graph-target
boundary.

## Working Conclusion

The project should now treat the admitted slice as:

- engine-owned for resident cell storage, mutable computational state, and
  graph/wiring target decisions
- Calc-owned for mutation entry, formula-cell object lifetime, live container
  residency, and final rollback

That is a meaningful additional boundary shift, but it remains intentionally
narrow.
