# Computational Substrate Off-Sheet Named-Range And Merge Carry-Through Decision Record

Status: completed closeout for the bounded off-sheet named-range and merge pass

## Decision

Admit the bounded off-sheet named-range-combined shared-group surface on the
new `GlobalSingleAreaSingleConsumerSheet` boundary for:

- `SameTextPreserve` `SetFormula`
- `Regroup` `SetFormula`
- `OneSidedInsert` `SetFormula`

Keep the following deferred:

- off-sheet named-range-combined `MemberExit`
- direct off-sheet gap merge and replacement-merge
- off-sheet multi-group collapse
- multi-consumer-sheet and workbook-wide off-sheet authority

## Why

The key blocker was that the facade collapsed every off-sheet named-range-
combined case into generic `Deferred`, which kept the authority gate from
ever considering the bounded off-sheet lane.

This pass fixed that by introducing an explicit bounded off-sheet named-
range boundary and then proving that the already-existing exact carry-
through machinery closes on that lane for:

- preserve
- regroup
- one-sided insert

The runtime did not need a new off-sheet planner beyond the bounded
boundary recognition and reuse of the existing named-range observation-build
adjustments.

## Evidence Summary

The pass added or updated proof for:

- facade classification of bounded off-sheet named-range-combined candidates
- live Calc host-shape proof for off-sheet named-range preserve, regroup,
  and one-sided insert
- standalone exactness proof for those same families
- live narrow-rollout authority/lifecycle/mutation-entry proof on the
  bounded lane

## Current Off-Sheet Boundary

The admitted off-sheet boundary now includes:

- direct one-consumer-sheet `MemberExit`
- direct one-consumer-sheet `SameTextPreserve`
- direct one-consumer-sheet `Regroup`
- named-range-combined one-consumer-sheet
  `GlobalSingleAreaSingleConsumerSheet` `SameTextPreserve`
- named-range-combined one-consumer-sheet
  `GlobalSingleAreaSingleConsumerSheet` `Regroup`
- named-range-combined one-consumer-sheet
  `GlobalSingleAreaSingleConsumerSheet` `OneSidedInsert`

The retained off-sheet boundary is now:

- named-range-combined `MemberExit`
- direct gap merge and replacement-merge
- off-sheet multi-group collapse
- multi-consumer-sheet and workbook-wide authority

## Next Logical Blocker

The next off-sheet blocker is narrower than before:

- exact off-sheet named-range-combined `MemberExit`
- exact direct off-sheet gap merge and replacement-merge

After that, any remaining off-sheet work should stay fenced to bounded
consumer-sheet surfaces rather than reopening a broad workbook-wide blocker.
