# Computational Substrate Shared-Group Named-Range Member-Exit Contract

Status: frozen contract for bounded named-range-combined scalar member-exit admission

## Admitted Family

This cycle admits only:

- same-sheet shareable shared-group `SetScalarValue`
- shared-formula mutation family `MemberExit`
- named-range boundary `GlobalSingleAreaSameSheet`

## Exactness Requirements

The family is in contract only when all of the following hold:

- named-range descriptors are stable before and after
- the touched shared-group member becomes a scalar cell
- the surviving shared-group members remain one exact contiguous run
- dependency snapshot construction stays non-opaque
- queue, graph, and IR verification close exactly on the live admitted
  path

## Live Ownership Requirement

The live path owns this family only when:

- graph-after is projected from the predicted computational after-shadow on
  the same canonical builder used by verification
- live expected IR is compiled from the predicted computational after-shadow
  with Calc's compile host before comparing to the live after-state

## Out Of Contract

The following remain out of contract in this cycle:

- named-range-combined `SetFormula` member-exit
- named-range-combined `ClearCell` member-exit
- named-range-combined regroup, merge, and collapse
- repair-sensitive normalization
- off-sheet shared-group behavior
