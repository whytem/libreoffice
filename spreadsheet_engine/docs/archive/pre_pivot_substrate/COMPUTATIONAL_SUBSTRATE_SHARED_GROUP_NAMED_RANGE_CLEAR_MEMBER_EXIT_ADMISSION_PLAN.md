# Computational Substrate Shared-Group Named-Range Clear Member-Exit Admission Plan

Status: completed closeout plan for bounded named-range-combined
`ClearCell` member-exit admission

## Closeout Result

This plan is now complete.

The cycle widened the admitted slice by one bounded step:

- same-sheet shareable shared-group named-range-combined `ClearCell`
  `MemberExit` is now admitted
- only when the named-range boundary is
  `GlobalSingleAreaSameSheet`

## Purpose

This plan defined the next bounded admission pass after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_DECISION_RECORD.md).

At entry, bounded named-range-combined preserve, scalar member-exit, and
`SetFormula` member-exit were already admitted, but bounded `ClearCell`
member-exit still stopped at a planner-authored after-graph mismatch.

## Plan Goal

Remove that bounded planner blocker and answer one explicit question:

- can the engine admit same-sheet shareable named-range-combined
  `ClearCell` `MemberExit` exactly through authority, lifecycle, and
  mutation entry

## Candidate Support Slice

The cycle stayed intentionally narrow:

- same-sheet shareable shared-group `ClearCell`
- shared-formula mutation family: `MemberExit`
- named-range boundary: `GlobalSingleAreaSameSheet`
- stable named-range descriptors before and after
- touched member exits the group and becomes empty
- surviving group remains one contiguous run
- no off-sheet consumers
- no repair-sensitive normalization

## Non-Goals

This plan does not attempt to:

- widen named-range-combined regroup, merge, or collapse
- widen repair-sensitive or off-sheet behavior
- weaken exactness into normalization-only acceptance
- admit broader non-edge shared-group classes

## Required Deliverables

This cycle is complete only when all of the following exist:

1. a checked-in contract for bounded named-range-combined `ClearCell`
   member-exit
2. a checked-in scenario matrix separating admit and retained defer buckets
3. a checked-in mapping note for the planner-authored after-graph rule
4. runtime support for exact bounded `ClearCell` member-exit planning
5. standalone exactness proof for bounded named-range-combined `ClearCell`
   member-exit
6. live authority, lifecycle, and mutation-entry apply proof for the same
   lane
7. updated implementation and evidence notes
8. a checked-in decision record

## Workstreams

### 1. Freeze The Clear Member-Exit Contract

Define the exact admitted lane before changing runtime behavior.

### 2. Land The Planner Fix

Make the bounded authority observation surface match live Calc exactly for
the named-range-combined `ClearCell` lane.

### 3. Re-run The Candidate

Re-run standalone and live proof on the same bounded surface.

### 4. Close Out The Cycle

Record the newly admitted lane and the remaining deferred frontier.
