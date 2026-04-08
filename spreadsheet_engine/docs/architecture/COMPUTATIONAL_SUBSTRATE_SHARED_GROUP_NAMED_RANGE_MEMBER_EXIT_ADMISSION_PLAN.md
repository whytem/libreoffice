# Computational Substrate Shared-Group Named-Range Member-Exit Admission Plan

Status: completed closeout plan for bounded named-range-combined scalar member-exit admission

## Closeout Result

This plan is now complete.

The cycle widened the admitted slice by one bounded step:

- same-sheet shareable shared-group named-range-combined
  `SetScalarValue` `MemberExit` is now admitted
- only when the named-range boundary is
  `GlobalSingleAreaSameSheet`

The broader named-range-combined member-exit frontier does not admit in
this pass. `SetFormula` and `ClearCell` member-exit remain deferred.

## Purpose

This plan defined the next blocker-removal and admission cycle after the
completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_DECISION_RECORD.md).

At entry, bounded named-range-combined preserve was already admitted, but
topology-changing named-range-combined member-exit still did not close in
the live admitted path.

The exact blocker was narrower than shared-group topology prediction:

- standalone bounded member-exit prediction already closed exactly
- live authority still rolled back on verification
- the remaining mismatch sat in expected graph and IR projection for the
  bounded live surface

## Plan Goal

Remove that blocker and answer one explicit question:

- can the engine admit bounded same-sheet shareable named-range-combined
  scalar member-exit exactly through authority and mutation entry

## Candidate Support Slice

The cycle stayed intentionally narrow:

- same-sheet shareable shared-group `SetScalarValue`
- shared-formula mutation family: `MemberExit`
- named-range boundary: `GlobalSingleAreaSameSheet`
- stable named-range descriptors before and after
- touched member exits the group and becomes scalar
- surviving group remains one contiguous run
- no off-sheet consumers
- no repair-sensitive normalization

## Non-Goals

This plan does not attempt to:

- admit named-range-combined `SetFormula` member-exit
- admit named-range-combined `ClearCell` member-exit
- widen named-range-combined regroup, merge, or collapse
- widen repair-sensitive or off-sheet behavior
- weaken exactness into normalization-only acceptance

## Required Deliverables

This cycle is complete only when all of the following exist:

1. a checked-in contract for the bounded scalar member-exit surface
2. a checked-in scenario matrix for admit and retained reject buckets
3. a checked-in mapping note for live graph and IR ownership
4. runtime support for exact live graph and IR verification on the bounded
   slice
5. standalone exactness proof for bounded scalar member-exit
6. live authority and mutation-entry apply proof on the same slice
7. retained reject proof for named-range-combined `SetFormula` and
   `ClearCell` member-exit
8. a checked-in evidence note and decision record

## Workstreams

### 1. Freeze The Scalar Member-Exit Contract

Define the exact admitted family before changing runtime behavior.

### 2. Freeze The Scenario Matrix

Separate the bounded scalar admit bucket from retained non-scalar reject
buckets.

### 3. Freeze The Mapping Rules

Define exact live graph and IR ownership for the bounded slice.

### 4. Land Runtime Support

Make the live authority and mutation-entry verification surfaces exact for
the bounded scalar member-exit family.

### 5. Re-run The Candidate

Re-run standalone and live proof for the bounded scalar member-exit family.

### 6. Close Out The Cycle

Record the admitted boundary and the retained defer list.
