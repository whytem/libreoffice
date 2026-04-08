# Computational Substrate Shared-Group Named-Range Non-Scalar Member-Exit Admission Plan

Status: completed closeout plan for bounded named-range-combined
non-scalar member-exit admission

## Closeout Result

This plan is now complete.

The cycle widened the admitted slice by one bounded step:

- same-sheet shareable shared-group named-range-combined `SetFormula`
  `MemberExit` is now admitted
- only when the named-range boundary is
  `GlobalSingleAreaSameSheet`

The adjacent `ClearCell` lane did not admit in the same pass. It remains
deferred because the authority planner still authors a predicted
broadcaster and dependency-edge surface that does not match live Calc
after-state exactly.

## Purpose

This plan defined the next bounded admission pass after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_DECISION_RECORD.md).

At entry, bounded named-range-combined preserve and scalar member-exit were
already admitted, but non-scalar member-exit still stopped at the deferred
boundary.

The next explicit questions were:

- can the engine admit bounded named-range-combined `SetFormula`
  `MemberExit` exactly
- can the adjacent bounded named-range-combined `ClearCell`
  `MemberExit` close on the same exact authority model

## Plan Goal

Land the narrowest exact non-scalar named-range member-exit expansion
possible without widening into regroup, merge, collapse, repair-sensitive,
or off-sheet behavior.

## Candidate Support Slice

The cycle stayed intentionally narrow:

- same-sheet shareable shared-group named-range-combined
  `MemberExit`
- named-range boundary: `GlobalSingleAreaSameSheet`
- stable named-range descriptors before and after
- touched member exits the group
- surviving group remains one contiguous run
- first target: `SetFormula` member-exit
- second target: `ClearCell` member-exit
- no off-sheet consumers
- no repair-sensitive normalization

## Non-Goals

This plan does not attempt to:

- widen named-range-combined regroup, merge, or collapse
- widen repair-sensitive or off-sheet behavior
- weaken exactness into normalization-only acceptance
- admit `ClearCell` if the live authority graph remains non-exact

## Required Deliverables

This cycle is complete only when all of the following exist:

1. a checked-in contract for bounded non-scalar named-range member-exit
2. a checked-in scenario matrix separating admit and retained defer
   buckets
3. a checked-in mapping note for the predicted-after ownership rule
4. runtime support for bounded exact `SetFormula` member-exit
5. standalone exactness proof for bounded named-range-combined
   `SetFormula` member-exit
6. live authority, lifecycle, and mutation-entry proof for bounded
   named-range-combined `SetFormula` member-exit
7. retained reject proof for bounded named-range-combined `ClearCell`
   member-exit
8. checked-in evidence and decision docs

## Workstreams

### 1. Freeze The Non-Scalar Contract

Define the bounded non-scalar named-range member-exit surface before
changing runtime behavior.

### 2. Land `SetFormula` First

Widen the admitted gate only as far as exact named-range-combined
`SetFormula` member-exit closes cleanly.

### 3. Re-run `ClearCell` On The Same Surface

Probe the adjacent `ClearCell` lane on the same bounded surface and keep it
deferred unless the authority planner reaches exact graph and IR
verification.

### 4. Close Out The Cycle

Record the admitted `SetFormula` lane and the retained `ClearCell`
defer boundary.
