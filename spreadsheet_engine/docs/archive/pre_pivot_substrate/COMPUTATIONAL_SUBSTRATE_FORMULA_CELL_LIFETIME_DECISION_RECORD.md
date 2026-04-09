# Computational Substrate Formula-Cell Lifetime Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-owned formula-cell lifetime on the admitted slice.

Do not treat this as proof for broad `ScDocument` object-lifetime migration,
direct engine-owned mutation entry, or broad formula-cell migration outside
the admitted slice.

## What Is Now Justified

The bounded admitted slice now has checked-in proof for:

- engine-owned resident cell storage
- engine-owned resident wiring containers
- engine-owned mutable computational state
- engine-owned graph, wiring, and queue decisions
- engine-owned admitted formula-cell lifetime records and after-state
  decisions
- Calc-side realization of admitted live `ScFormulaCell` objects from that
  engine-owned lifetime state with exact computational, graph, and queue
  verification

That is strong enough to treat admitted-slice formula-cell lifetime as a real
engine-owned boundary surface instead of a retained Calc-only responsibility.

## What Is Still Not Justified

This closeout does not justify:

- direct engine-owned document mutation entry
- broad formula-cell object migration outside the admitted slice
- shared-group-sensitive lifecycle or structural migration
- named-range-sensitive structural rollout
- sheet-level structural edits
- token-container ownership migration
- broad document-host storage transfer

Calc still remains the host for:

- document mutation APIs
- live object realization
- final rollback

## Next Adjacent Concern

Because admitted-slice resident cells, resident wiring, and formula-cell
lifetime are now proven, the next adjacent migration concern is:

- direct admitted-slice mutation entry

That should come before any broader claim about `ScDocument` object-lifetime
or mutation authority transfer.

## Working Conclusion

The project should now treat the admitted slice as:

- engine-owned for resident cell storage, resident wiring containers, mutable
  computational state, graph/wiring/queue decisions, and formula-cell
  lifetime decisions
- Calc-owned for mutation entry, live object realization, and final rollback

That is a meaningful additional boundary shift, but it remains intentionally
narrow.
