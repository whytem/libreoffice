# Computational Substrate Shared-Group Named-Range Member-Exit Mapping Rules

Status: frozen mapping rules for bounded named-range-combined scalar member-exit admission

## Boundary Mapping

- named-range boundary is classified from the before and after facades
- only `GlobalSingleAreaSameSheet` is in-bounds
- only `MemberExit` paired with `SetScalarValue` is admitted

## Graph Mapping

- authority graph-after is derived from the predicted computational
  after-shadow with the canonical dependency-graph shadow builder
- broadcaster-node counts therefore come from the predicted broadcaster
  surface instead of a second bespoke reconstruction path

## IR Mapping

- live verification compares Calc-hosted IR compiled from the predicted
  computational after-shadow against Calc-hosted IR from the live
  after-shadow
- standalone authority exactness still keeps the in-memory authority IR
  proof bucket

## Reject Mapping

- if the family is `MemberExit` but the mutation kind is not
  `SetScalarValue`, the boundary remains
  `shared_group_named_range_out_of_contract`
