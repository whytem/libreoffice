# Computational Substrate Off-Sheet Final Surface Plan

Status: completed closeout for the remaining bounded off-sheet blocker

## Objective

Close the last bounded off-sheet widening blocker on the already-admitted
one-consumer-sheet surface.

This pass targeted the two remaining families called out in the master
document:

- off-sheet named-range-combined `MemberExit`
- direct off-sheet gap merge and replacement-merge

The pass stays fenced to:

- same workbook
- one mutation sheet
- at most one additional consumer sheet
- exact queue, computational, graph, and IR verification
- exact authority, lifecycle, and mutation-entry carry-through

## Primary Target Families

The first target is bounded off-sheet named-range-combined
`GlobalSingleAreaSingleConsumerSheet` `MemberExit` for:

- `SetScalarValue`
- `SetFormula`
- `ClearCell`

The second target is bounded direct off-sheet merge-shaped shared-group
behavior without named-range-combined widening:

- `Merge` `SetFormula`
- `ReplacementMerge` `SetFormula`

## Explicitly Out Of Scope

This pass does not attempt to admit:

- multi-consumer-sheet off-sheet behavior
- workbook-wide off-sheet authority
- off-sheet named-range-combined multi-group collapse
- off-sheet named-range-combined descriptor drift
- off-sheet structural named-range rollout
- off-sheet repair-sensitive normalization

If any candidate does not close exactly, the pass should close with an
explicit retained blocker instead of widening scope.

## Why This Next

The current off-sheet surface is already bounded and exact for:

- direct `MemberExit`
- direct `SameTextPreserve`
- direct `Regroup`
- named-range-combined `SameTextPreserve`
- named-range-combined `Regroup`
- named-range-combined `OneSidedInsert`

That leaves one final bounded off-sheet frontier before any broader
multi-consumer or workbook-wide reconsideration:

- named-range-combined `MemberExit`
- direct merge-shaped carry-through

These are the highest-value next families because they remain on the same
already-proven dependency surface.

## Phase 1: Freeze The Pass

Deliverables:

- this plan
- master/status/readme links
- explicit bounded target families

Success criteria:

- the pass isolates the final bounded off-sheet surface
- broader off-sheet authority remains explicitly out of scope

## Phase 2: Add Missing Proof Buckets

Work:

- add facade classification proof for off-sheet named-range `MemberExit`
- add live Calc host-shape proof for off-sheet named-range `MemberExit`
- add facade or host-shape proof for direct off-sheet gap merge and
  replacement-merge
- add standalone exactness proof for every admitted candidate

Success criteria:

- each candidate family has frozen expected host shape
- exact standalone proof exists before live narrow-rollout proof is trusted

## Phase 3: Carry Through Runtime

Work:

- run authority, lifecycle, and mutation-entry proof on the bounded
  off-sheet named-range `MemberExit` families
- run authority, lifecycle, and mutation-entry proof on the bounded direct
  off-sheet merge and replacement-merge families
- add runtime changes only if proof exposes a real exactness gap

Success criteria:

- bounded candidates either apply exactly or fail with a narrower explicit
  blocker
- no broader off-sheet scope is pulled in to rescue one bounded family

## Phase 4: Close The Off-Sheet Surface

Deliverables:

- updated consolidated docs
- final decision summary
- explicit next frontier after this pass

Success criteria:

- the admitted off-sheet slice and any retained off-sheet defer are easy to
  read from the master doc
- the remaining frontier is no longer the old combined member-exit plus
  direct-merge blocker

## Outcome

This pass closed with a split result:

- bounded off-sheet named-range-combined
  `GlobalSingleAreaSingleConsumerSheet` `MemberExit` is admitted for
  `SetScalarValue`, `SetFormula`, and `ClearCell`
- bounded direct off-sheet replacement-merge does not remain a distinct
  blocker because live Calc normalizes the bounded attempt onto the already-
  admitted `Regroup` surface
- bounded direct off-sheet gap-closing insertion remains explicitly
  retained because live Calc exposes merged after-topology but does not
  surface a stable mutation-family classification for the bounded attempt

The completed closeout is recorded in:

- [COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_EVIDENCE.md)
