# Computational Substrate Storage And Wiring Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authoritative mutable sidecar state and engine-authored
graph/wiring targets on the admitted slice.

Do not treat this as proof for broad physical storage or listener-container
migration.

## What Is Now Justified

The bounded admitted slice now has checked-in proof for:

- engine-owned mutable computational sidecar state
- engine-owned listener/broadcaster, formula-tree, and formula-track target
  sets
- Calc-side replay of that admitted live target with exact graph verification

That is strong enough to treat the mutable sidecar and graph/wiring target
surface as a real engine-owned authority boundary on the admitted slice.

## What Is Still Not Justified

This closeout does not justify:

- broad `ScDocument` storage migration
- broad broadcaster/listener container residency migration
- shared-group-sensitive lifecycle or structural migration
- named-range-sensitive structural rollout
- sheet-level structural edits
- token-container ownership migration

## Next Adjacent Concern

Because the authority path is now proven, the next adjacent migration concern
is:

- physical cell-storage residency migration on the admitted slice

That should come before any claim of broad listener/broadcaster container
residency transfer. The retained host containers can continue to mirror the
engine-owned target state while cell-storage residency is evaluated.

## Working Conclusion

The project should now treat the admitted slice as:

- engine-owned for mutable sidecar state and graph/wiring target decisions
- Calc-owned for physical container residency, mutation entry, and rollback

That is a meaningful boundary shift, but still a narrow one.
