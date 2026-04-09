# Computational Substrate Final Rollback Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authored admitted-slice rollback on the bounded slice.

This is a bounded proceed decision, not a broad document-host transplant.

## What The Evidence Justifies

The completed proof cycle now justifies one additional settled boundary shift
on the admitted slice:

- the engine already owns resident cell storage
- the engine already owns resident wiring containers
- the engine already owns admitted formula-cell lifetime decisions
- the engine already owns admitted scalar mutation-entry request shape,
  routing, and after-state decisions
- the engine already owns the admitted live object-realization record
- the engine now also owns the admitted rollback record consumed by Calc for
  rollback restore

The admitted exact result is stronger than the earlier hybrid rollback shape
because rollback no longer reconstructs before-state implicitly at the moment
it is needed. The admitted mutation-entry path now reuses one explicit
engine-authored rollback surface.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- raw document mutation APIs
- the final live apply shell that executes engine-authored realization and
  rollback records
- all workbook and mutation classes outside the admitted slice

That remaining host role is now narrower than before this cycle, but it is
still real.

## Why This Is A Proceed Rather Than Hybrid

The evidence supports proceed because all of the following now hold on the
admitted slice:

- helper-level rollback from an explicit rollback record closes exact
- mutation-entry dirty-baseline rejection now records explicit exact
  rollback observation
- missing restored objects are diagnosed explicitly as
  `missing_restored_objects`
- host-only rollback reconstruction remains isolated in the observation model
- queue, computational, graph, and replay baselines remain green

This is strong enough to treat admitted rollback records as part of the
settled engine-authored boundary on the bounded slice.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive rollback
- named-range-sensitive rollback
- off-sheet or sheet-wide structural rollback
- broad raw mutation API migration
- broad `ScDocument` host independence

## Next Adjacent Concern

The next adjacent concern after this closeout is:

- broader raw mutation API migration on the admitted slice

The engine now owns the admitted resident, mutation-entry, realization, and
rollback records that were the main prerequisites for that question. What
remains host-owned on the bounded slice is no longer rollback authority
itself. It is the retained raw mutation shell and the final live apply shell
around it.
