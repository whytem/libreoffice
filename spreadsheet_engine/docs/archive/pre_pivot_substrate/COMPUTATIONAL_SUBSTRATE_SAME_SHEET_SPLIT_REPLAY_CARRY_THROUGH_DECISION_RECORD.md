# Computational Substrate Same-Sheet Split Replay Carry-Through Decision Record

Status: completed closeout for the direct same-sheet split-backed replay pass

## Decision

Admit the bounded same-sheet named-range split-backed three-group
`SetFormula` replay lane on the `GlobalSingleAreaSameSheet` surface.

This is not a new one-group collapse family.

It is the exact live split-backed named-range `Regroup` host shape, now
admitted on:

- authority replay
- lifecycle replay
- mutation-entry

## Why

The prior split-outcome pass had already established three things:

- the live host shape is `Regroup` plus `Split`, not one-group collapse
- standalone exactness already closes on that host shape
- mutation-entry already carries that host shape exactly

What was still missing was direct replay exactness.

This pass isolated the mismatch to one exact seam:

- broadcaster/listener attachment on the named-range area broadcaster

Queue, predicted topology, formula groups, named-range descriptors,
execution IR, graph nodes, and listener-anchor sets were already exact.

The final bounded fix taught direct replay to mirror the actual live
same-sheet split-backed named-range broadcaster shape:

- the touched surviving after-group uses top-cell plus touched-cell
  area-listener replay
- the far surviving after-group uses group-listener plus top-cell
  augmentation

Once that shaped replay surface was in place, authority and lifecycle both
verified exactly.

## Exact Result

The bounded same-sheet named-range three-group split-backed attempt now
closes as:

- live Calc host shape: `Regroup` plus `Split`
- standalone exactness: exact
- authority replay: exact apply
- lifecycle replay: exact apply
- mutation-entry: exact apply

## Current Same-Sheet Boundary

The same-sheet named-range-combined admitted boundary now includes:

- bounded `SameTextPreserve`
- bounded `Regroup`
- bounded `OneSidedInsert`
- bounded `MemberExit`
- bounded split-backed three-group `SetFormula` replay on the
  `GlobalSingleAreaSameSheet` surface

The old same-sheet split replay blocker is gone.

The retained same-sheet boundary is now narrower still:

- named-range-sensitive structural rollout
- shared-group-sensitive structural behavior outside the bounded exact
  same-sheet shareable slice

## Next Logical Blocker

The next material admitted-slice blocker is no longer same-sheet replay.

The next narrow expansion target is the retained direct off-sheet
gap-closing insertion surface where:

- live Calc merges the off-sheet after-topology
- but the mutation-family surface is still left at `None`

That is the next highest-value bounded blocker to address.
