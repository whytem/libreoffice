# Computational Substrate Shared-Group Named-Range Live Ownership Plan

Status: completed blocker-removal plan for bounded named-range-combined live ownership

## Closeout Result

This plan is now complete.

The cycle removed the remaining bounded named-range-combined preserve
blocker and widened the admitted slice by one bounded step:

- dependency snapshot construction no longer reports opacity on bounded
  named-range target expressions
- live lifecycle now applies the bounded preserve family
- live mutation entry now applies the same bounded preserve family

The admitted shared-group slice now includes same-sheet shareable
named-range-combined `SameTextPreserve` on the
`GlobalSingleAreaSameSheet` boundary.

## Purpose

This document defines the next explicit blocker-removal cycle after the
completed
[COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_DECISION_RECORD.md).

The bounded same-sheet shareable named-range-combined `SameTextPreserve`
candidate no longer fails because listener-anchor kinds are unsupported.
It still does not admit because the engine does not yet own the live
named-range-combined surface end to end:

- lifecycle still reaches `opaque_dependency_surface`
- mutation entry still reaches `rollback_queue_or_state_mismatch`
- synthetic exact-restore proof buckets still do not close exactly

This cycle isolates that remaining live ownership gap instead of mixing it
with broader named-range-combined widening, repair-sensitive normalization,
or off-sheet promotion.

## Plan Goal

Remove the currently settled blocker on the bounded named-range-combined
preserve family by teaching the live admitted-slice pipeline to own:

- the bounded named-range-combined dependency surface without opacity
- exact carry-through into live apply, rollback, and final verification

The closeout must answer one explicit question:

- can the engine now carry the bounded same-sheet global-single-area
  named-range-combined `SameTextPreserve` family through lifecycle and
  mutation entry without `opaque_dependency_surface` or
  `rollback_queue_or_state_mismatch`

## Entry Boundary

This plan begins from the currently settled boundary:

- shared-group structural and bounded non-structural admitted families are
  unchanged
- true `FormulaGroup` listener anchors are live-owned on already-admitted
  shared-group paths
- bounded named-range-combined `SameTextPreserve` closes exact standalone
  computational, graph, and IR proof
- bounded named-range-combined member-exit remains out of contract
- live lifecycle and mutation entry still do not admit the preserve family

The current blocker is explicit in:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- [ComputationalSubstrateLiveApply.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLiveApply.hxx)
- [ComputationalSubstrateFinalVerification.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFinalVerification.hxx)

## Candidate Support Slice

The first implementation slice must stay narrow.

The required support target is:

- same-sheet shareable shared-group `SetFormula`
- named-range boundary classification:
  `GlobalSingleAreaSameSheet`
- mutation family:
  `SameTextPreserve`
- no named-range descriptor drift
- no off-sheet consumers
- no repair-sensitive normalization
- no member-exit, regroup, merge, or collapse in the same pass

Representative blocked shape:

- a bounded `=COUNTA(Metrics)+A2` same-text preserve edit inside a
  shareable group
- the named range `Metrics` remains a stable global single-area same-sheet
  descriptor
- standalone prediction closes exactly
- live lifecycle rejects at `opaque_dependency_surface`
- live mutation entry rejects at `rollback_queue_or_state_mismatch`

## Non-Goals

This plan does not attempt to:

- admit broader named-range-combined member-exit, regroup, merge, or
  multi-group-collapse classes
- widen off-sheet or repair-sensitive behavior
- weaken exactness into normalization-only acceptance
- admit `HostUnknown` listener anchors
- claim ownership of arbitrary hidden Calc dependency orchestration outside
  the bounded preserve slice

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in named-range live-ownership contract note
2. a checked-in scenario matrix for the bounded preserve slice
3. a checked-in mapping note for the non-opaque dependency and verification
   surface
4. runtime support that removes the live `opaque_dependency_surface`
   blocker for the bounded preserve slice or narrows it to a smaller
   explicit blocker with proof
5. standalone and live proof for the post-change lifecycle, live apply,
   rollback, and final-verification surface
6. rerun evidence on the bounded named-range-combined preserve family
7. a checked-in evidence note and decision record

## Workstreams

### 1. Freeze The Named-Range Live-Ownership Contract

Define the exact bounded ownership surface before changing runtime behavior.

This contract should freeze:

- the admitted candidate family:
  same-sheet global-single-area named-range-combined `SameTextPreserve`
- the dependency-surface requirement:
  no opaque nodes or edges on the bounded preserve slice
- the verification requirement:
  exact live apply, rollback, and final verification
- the retained defer list:
  member-exit, regroup, merge, collapse, repair-sensitive, off-sheet

### 2. Freeze The Scenario Matrix

Define the exact proof buckets the cycle must cover.

This matrix should separate:

- standalone bounded preserve exactness
- live lifecycle bounded preserve admission
- live mutation-entry bounded preserve admission
- rollback and final-verification proof on the same bounded slice
- retained reject proof for named-range-combined member-exit
- retained reject proof for off-sheet and repair-sensitive cases

### 3. Freeze The Mapping Rules

Define how the bounded live dependency surface is represented and compared.

This mapping note should state:

- how named-range descriptors are matched across before, predicted, and
  observed-after state
- what dependency-snapshot opacity reasons are still forbidden
- what counts as exact rollback and final-verification closure
- which live-only differences remain out of contract

### 4. Surface The Exact Live Blocker

Add proof that exposes the current bounded failure precisely enough to fix.

This workstream should cover:

- captured-shadow dependency-snapshot proof for the bounded preserve slice
- explicit issue-detail evidence for the current opaque surface
- explicit live-apply and rollback/final-verification reason capture

### 5. Land Runtime Support

Teach the bounded live path to own the currently opaque preserve surface.

This workstream should cover:

- bounded dependency-snapshot construction or canonicalization
- any required named-range or formula-source normalization that preserves
  exact semantics
- bounded live-apply, rollback, and final-verification support on the same
  preserve slice

### 6. Re-run The Blocked Admission Candidate

After the blocker-removal support lands, rerun the bounded preserve family:

- same-sheet shareable named-range-combined `SameTextPreserve`

This step does not automatically admit broader named-range-combined
behavior. It only determines whether the bounded preserve family now closes
exactly enough for admission.

### 7. Close Out The Cycle

Write the final evidence and decision record.

The closeout must state explicitly:

- whether the bounded named-range-combined preserve family is now admitted
- whether the blocker was removed entirely or only narrowed
- whether rollback and final verification are now exact on that slice
- what the next adjacent concern becomes after this cycle

## Recommended Execution Order

1. freeze the contract, scenario matrix, and mapping rules
2. add proof that exposes the exact bounded live opacity and rollback gap
3. land the runtime blocker-removal changes
4. rerun standalone and live bounded preserve proof
5. rerun retained reject proof for named-range member-exit and off-sheet
6. write evidence and decision closeout
