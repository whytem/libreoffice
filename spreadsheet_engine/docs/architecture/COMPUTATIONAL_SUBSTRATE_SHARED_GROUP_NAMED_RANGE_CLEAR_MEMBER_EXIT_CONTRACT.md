# Computational Substrate Shared-Group Named-Range Clear Member-Exit Contract

Status: frozen contract for bounded named-range-combined `ClearCell`
member-exit closeout

## Admitted Surface

This closeout admits only the following family:

- same-sheet shareable shared-group named-range-combined `ClearCell`
  `MemberExit`
- named-range boundary:
  `SharedFormulaNamedRangeMutationBoundary::GlobalSingleAreaSameSheet`
- stable named-range descriptors before and after
- touched member exits the group and becomes empty
- surviving group remains one contiguous run
- no off-sheet named-range consumers
- no repair-sensitive normalization

## Exactness Standard

The lane is admitted only when the engine can author all of the following
exactly from its own predicted after-state:

- surviving shared-group identity
- formula-tree and formula-track participation
- broadcaster nodes and listener-anchor edges
- dependency-graph projection
- Calc-hosted IR compiled from the predicted after-shadow

## Retained Deferred Boundary

This closeout does not admit:

- broader named-range-combined regroup, merge, or collapse
- repair-sensitive host normalization
- off-sheet shared-group behavior
- broader non-edge regroup and merge families
