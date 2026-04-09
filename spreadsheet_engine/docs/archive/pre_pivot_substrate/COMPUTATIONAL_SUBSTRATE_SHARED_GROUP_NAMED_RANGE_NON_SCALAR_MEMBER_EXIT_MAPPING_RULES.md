# Computational Substrate Shared-Group Named-Range Non-Scalar Member-Exit Mapping Rules

Status: frozen mapping note for bounded named-range-combined non-scalar
member-exit closeout

## Exact Ownership Rule

Bounded named-range-combined `SetFormula` `MemberExit` admits only when the
engine can author all of the following exactly from its own predicted
after-state:

- surviving shared-group identity
- formula-tree and formula-track participation
- broadcaster nodes and listener-anchor edges
- Calc-hosted IR compiled from the predicted after-shadow

## Why `SetFormula` Closes

For bounded `SetFormula` member-exit, the predicted after-shadow,
dependency snapshot, broadcaster mapping, graph projection, and IR
projection all align exactly with the live host after-state.

## Why `ClearCell` Still Does Not Close

For bounded `ClearCell` member-exit, the live broadcaster surface itself is
not the blocker. The retained mismatch sits in the authority planner’s
predicted broadcaster-node and edge projection.

That means `ClearCell` stays deferred until the planner can author the same
named-range-combined after-graph that live Calc exposes after the clear.
