# Computational Substrate Shared-Group Named-Range Non-Scalar Member-Exit Contract

Status: frozen contract for bounded named-range-combined non-scalar
member-exit closeout

## Admitted Lane

This closeout admits exactly one new family:

- same-sheet shareable shared-group named-range-combined `SetFormula`
  `MemberExit`
- named-range boundary:
  `SharedFormulaNamedRangeMutationBoundary::GlobalSingleAreaSameSheet`

The admitted lane requires all of the following:

- named-range descriptors remain stable before and after
- the touched cell starts inside one shareable shared group
- the touched cell exits that group and becomes an ordinary formula cell
- the surviving shared members remain one contiguous run
- the dependency snapshot has no opaque nodes or edges
- authority, lifecycle, and mutation-entry all verify exact queue, graph,
  and IR state

## Evaluated But Not Admitted

This closeout also evaluates the adjacent bounded `ClearCell`
`MemberExit` lane on the same surface, but it does not admit it.

`ClearCell` remains out of contract for the live admitted slice until the
authority planner can author the exact predicted broadcaster-node and
dependency-edge graph that Calc produces after the host clear.

## Still Out Of Scope

This contract does not cover:

- named-range-combined regroup, merge, or collapse
- repair-sensitive normalization
- off-sheet named-range consumers
- non-shareable or cross-sheet shared groups
