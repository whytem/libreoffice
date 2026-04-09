# Computational Substrate Shared-Group Named-Range Live Ownership Contract

Status: frozen closeout contract for bounded named-range-combined live ownership

## Admitted Slice

This closeout admits exactly one new shared-group named-range-combined
family:

- same-sheet shareable shared-group `SetFormula`
- named-range boundary:
  `GlobalSingleAreaSameSheet`
- mutation family:
  `SameTextPreserve`

## Exactness Requirement

This family is admitted only when all of the following hold:

- named-range descriptors are stable across before and observed-after state
- dependency snapshot construction produces zero opaque nodes and zero
  opaque edges
- queue, computational shadow, dependency graph, and IR all verify exactly
- lifecycle and mutation-entry both apply without rollback

## Engine-Owned Live Rules

The admitted live-owned rules are:

- bare named-range target expressions may be interpreted directly as cell or
  range references when the general formula parser does not accept them
- shared-group formulas keep member-local direct references on
  `FormulaCell` listener anchors
- named-range-resolved shared dependencies project onto the owning
  `FormulaGroup` listener anchor

## Retained Deferred Boundary

This closeout does not admit:

- named-range-combined member-exit
- named-range-combined regroup, merge, or collapse
- repair-sensitive normalization
- off-sheet consumers
- `HostUnknown` listener anchors
