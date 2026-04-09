# Computational Substrate Off-Sheet Final Surface Decision Record

Status: completed closeout for the bounded off-sheet final-surface pass

## Decision

Admit the bounded off-sheet named-range-combined
`GlobalSingleAreaSingleConsumerSheet` `MemberExit` slice for:

- `SetScalarValue`
- `SetFormula`
- `ClearCell`

Do not admit a new distinct direct off-sheet merge family in this pass.

Instead:

- the bounded direct off-sheet replacement attempt closes as the already-
  admitted direct off-sheet `Regroup` surface
- the bounded direct off-sheet gap-closing insertion remains explicitly
  deferred because live Calc exposes merged after-topology but no stable
  mutation-family classification

## Why

The pass started with two remaining off-sheet questions:

- whether the named-range-combined off-sheet mutation could become
  `MemberExit`
- whether the bounded direct off-sheet surface could widen into distinct
  gap merge or replacement-merge

The named-range-combined result closed cleanly. The existing bounded
off-sheet machinery already carried the exact
`GlobalSingleAreaSingleConsumerSheet` `MemberExit` lane once the missing
proof buckets were added.

The direct merge-shaped result closed differently:

- live Calc does not surface the bounded replacement attempt as
  `ReplacementMerge`; it surfaces it as `Regroup`
- live Calc does not surface the bounded gap-closing insertion as `Merge`;
  it leaves the mutation-family classification at `None` even though the
  after-topology merges

That means only one of the original blockers was a real admitted-slice
expansion. The other two were resolved by host-shape evidence rather than
new family admission.

## Evidence Summary

This pass added or updated proof for:

- facade classification of off-sheet named-range-combined `MemberExit`
- live Calc host-shape proof for off-sheet named-range-combined
  `MemberExit`
- standalone exactness proof for off-sheet named-range-combined
  `MemberExit`
- live authority, lifecycle, and mutation-entry proof for off-sheet
  named-range-combined `MemberExit`
- live Calc host-shape proof showing direct off-sheet replacement attempts
  normalize to `Regroup`
- live Calc host-shape proof showing direct off-sheet gap-closing insertion
  remains host-uncategorized at the mutation-family layer

## Current Off-Sheet Boundary

The admitted one-consumer-sheet off-sheet boundary now includes:

- direct `MemberExit`
- direct `SameTextPreserve`
- direct `Regroup`
- named-range-combined `SameTextPreserve`
- named-range-combined `Regroup`
- named-range-combined `OneSidedInsert`
- named-range-combined `MemberExit`

The retained one-consumer-sheet off-sheet boundary is now narrower:

- direct gap-closing insertion where live Calc merges the after-topology
  but does not expose a stable mutation-family contract

Outside that bounded surface, the broader retained off-sheet frontier is
still:

- multi-consumer-sheet off-sheet behavior
- workbook-wide off-sheet authority
- off-sheet named-range-combined descriptor drift or collapse beyond the
  bounded exact slice

## Next Logical Blocker

The old combined off-sheet blocker is now closed.

If off-sheet work continues immediately, the next narrow off-sheet target is
the host-uncategorized direct gap-closing insertion surface. If the roadmap
re-prioritizes by materiality instead, the next broader blocker is the
retained same-sheet multi-group collapse boundary.
