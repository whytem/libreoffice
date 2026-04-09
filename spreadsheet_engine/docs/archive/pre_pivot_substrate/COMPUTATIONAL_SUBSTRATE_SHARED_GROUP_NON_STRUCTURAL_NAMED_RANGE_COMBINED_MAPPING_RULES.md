# Computational Substrate Shared-Group Non-Structural Named-Range-Combined Mapping Rules

Status: frozen mapping note for the bounded named-range-combined closeout

## Shared-Group Mapping

The named-range-combined preserve candidate inherits the exact preserve rules
from the earlier shared-group same-text-preserve cycle:

- touched address is shared before and after
- before and after shared-group anchors and lengths match exactly
- no regroup, split, rebuild, merge, or member-exit topology is normalized

## Named-Range Mapping

The named-range boundary is classified on the facade surface as one of:

- `None`
- `GlobalSingleAreaSameSheet`
- `Deferred`

For `GlobalSingleAreaSameSheet` the following must all hold:

- every relevant referenced descriptor resolves before and after
- descriptor identity is stable
- scope is global
- target is single-area
- all relevant named-range consumers stay on the mutation sheet

## Exactness Rule

For the preserve family under test, exact closure would require:

- exact shared-group preserve topology
- exact named-range descriptor inventory after the mutation
- exact named-range comparison match in computational verification
- exact graph and IR verification on the predicted after-state

## Reject Rule

If the named-range boundary is not `None`, the shared-group non-structural
authority candidate path now treats only one family as in-bounds for
prediction:

- `GlobalSingleAreaSameSheet` plus `SameTextPreserve`

All other named-range-combined mutation families reject with:

- `shared_group_named_range_out_of_contract`

That includes bounded named-range-combined member-exit, which is now an
explicit retained reject rather than an implicit gap.

## Closeout Result

The preserve-only candidate rule was not enough to widen the admitted live
slice. Live lifecycle and mutation-entry carry-through still remain outside
contract.
