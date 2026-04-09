# Computational Substrate Off-Sheet Gap Insertion Surface Decision Record

Status: completed closeout for the bounded direct off-sheet gap-closing
insertion pass

## Decision

Admit the bounded same-workbook one-consumer-sheet direct off-sheet
gap-closing insertion `SetFormula` lane.

This is not a new facade mutation-family label.

Live Calc still exposes this host shape as:

- merged after-topology on the mutation sheet
- workbook-facade mutation-family `None`

But the exact bounded lane is now admitted anyway because the actual
authority, lifecycle, and mutation-entry carry-through contracts all close
exactly on that host-shaped surface.

## Why

The retained blocker assumption was that direct off-sheet gap-closing
insertion could not be admitted until the live host surfaced a stable
mutation-family label.

This pass proved the narrower truth:

- lifecycle already carries the bounded host shape exactly
- mutation-entry already carries the bounded host shape exactly
- the missing authority proof also closes exactly once it is asserted
  against the real authority contract

That authority contract is:

- exact predicted recalc queue
- exact graph carry-through
- exact IR carry-through

It is not immediate user-visible consumer cache refresh on the authority
pilot alone. The user-visible value update is already covered by the
existing lifecycle and mutation-entry buckets for the same surface.

## Exact Result

The bounded direct off-sheet gap-closing insertion surface now closes as:

- live Calc workbook-facade host shape: mutation-family `None` with merged
  after-topology
- standalone exactness: exact
- authority replay: exact queue and graph/IR carry-through, including the
  inserted formula and the off-sheet consumer in the predicted queue
- lifecycle replay: exact apply
- mutation-entry: exact apply

## Current Off-Sheet Boundary

The admitted bounded one-consumer-sheet direct off-sheet surface now
includes:

- `MemberExit`
- `SameTextPreserve`
- `Regroup`
- host-uncategorized gap-closing insertion

The bounded one-consumer-sheet named-range-combined surface remains:

- `SameTextPreserve`
- `Regroup`
- `OneSidedInsert`
- `MemberExit`

The old narrow off-sheet gap-insertion blocker is gone.

## Next Logical Blocker

The next material admitted-slice blocker is no longer off-sheet.

The next roadmap item is the retained named-range-sensitive structural
rollout frontier.
