# Computational Substrate Object Realization Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authored admitted-slice object realization on the bounded
slice.

This is a bounded proceed decision, not a broad document-host transplant.

## What The Evidence Justifies

The completed proof cycle now justifies one additional settled boundary shift
on the admitted slice:

- the engine already owns resident cell storage
- the engine already owns resident wiring containers
- the engine already owns admitted formula-cell lifetime decisions
- the engine already owns admitted scalar mutation-entry request shape,
  routing, and after-state decisions
- the engine now also owns the admitted live object-realization record that
  Calc consumes for formula-cell, wiring, formula-tree, and formula-track
  realization

The admitted exact result is stronger than the earlier hybrid realization
shape because the live realization call sites are no longer coordinating
three unrelated resident stores implicitly. They now consume one explicit
engine-authored realization record.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- raw document mutation APIs
- final rollback after failed verification
- all workbook and mutation classes outside the admitted slice

That remaining host role is now narrower than before this cycle, but it is
still real.

## Why This Is A Proceed Rather Than Validation-Only

The evidence supports proceed because all of the following now hold on the
admitted slice:

- the engine-authored realization record closes exact admitted lifecycle
  realization
- direct admitted mutation-entry application now records an explicit
  object-realization observation
- applied admitted mutation-entry results classify object realization as
  `exact`
- missing realized live objects are diagnosed explicitly as
  `missing_realized_objects`
- queue, computational, graph, and replay baselines remain green

This is strong enough to treat admitted live object realization as part of
the settled engine-authored boundary on the bounded slice.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive realization
- named-range-sensitive realization
- off-sheet or sheet-wide structural realization
- rollback migration out of Calc
- broad raw mutation API migration
- broad `ScDocument` host independence

## Next Adjacent Concern

The next adjacent concern after this closeout is:

- final rollback reassessment

The engine now owns the admitted resident and realization surfaces that were
the main prerequisites for that question. What remains host-owned on the
bounded slice is no longer broad live object realization. It is the retained
rollback boundary and the raw mutation shell around it.
