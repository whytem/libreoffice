# Computational Substrate Formula-Group Listener-Anchor Live Support Plan

Status: completed blocker-removal plan for true `FormulaGroup` listener-anchor support

## Closeout Result

This plan is now complete.

The cycle landed true `FormulaGroup` listener-anchor replay on the live
wiring path for already-admitted shared-group surfaces while keeping
`HostUnknown` explicitly out of contract.

The bounded named-range-combined preserve family no longer fails because of
`listener_anchor_out_of_contract`. The remaining blockers are now:

- lifecycle `opaque_dependency_surface`
- mutation-entry `rollback_queue_or_state_mismatch`
- standalone exact-restore gaps on synthetic object-realization and
  rollback proof buckets

## Purpose

This document defines the next explicit blocker-removal cycle after the
completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_DECISION_RECORD.md).

The bounded named-range-combined preserve candidate no longer fails because
the shared-group or named-range facade boundary is unknown. It now fails
because the live carry-through path still does not own the listener-anchor
surface end to end:

- live observation can surface `FormulaGroup` and `HostUnknown` listeners
- the mutable wiring store can retain those listener anchors
- live wiring realization still rejects any listener anchor that is not a
  single `FormulaCell`
- mutation entry and final verification therefore stop at
  `listener_anchor_out_of_contract`

This plan isolates that blocker as its own proof cycle instead of mixing it
with broader named-range-combined promotion, repair-sensitive normalization,
or off-sheet widening.

## Plan Goal

Teach the live admitted-slice pipeline to support true
`FormulaGroup` listener anchors throughout:

- live observation
- wiring-container residency
- object realization
- rollback
- final verification
- mutation-entry carry-through

The closeout must answer one explicit question:

- can the engine carry `FormulaGroup` listener anchors through live wiring
  and verification exactly enough that currently blocked bounded families,
  starting with named-range-combined shared-group preserve, can be
  reconsidered for admission

## Entry Boundary

This plan begins from the currently settled boundary:

- shared-group structural and bounded non-structural admitted families are
  unchanged
- bounded named-range-combined preserve closes in standalone proof
- live lifecycle and mutation entry still do not admit that preserve family
- mutation entry reaches `listener_anchor_out_of_contract`

The current live blocker is explicit in:

- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateFinalVerification.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFinalVerification.hxx)
- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)

## Candidate Support Slice

The first implementation slice must stay narrow.

The required support target is:

- true `FormulaGroup` listener-anchor support for already-admitted same-sheet
  shareable shared-group live surfaces
- no broadening to arbitrary host-only listener kinds
- no weakening from exactness to “ignore the mismatch”
- exact carry-through for the wiring store, live apply, rollback, and final
  verification

Representative blocked shape:

- a bounded named-range-combined same-text preserve edit whose predicted
  standalone after-state is exact
- live listener/broadcaster observation includes a `FormulaGroup`
  listener-anchor shape on that same bounded family
- mutation entry fails because live wiring realization only accepts
  `FormulaCell` listener anchors

## Non-Goals

This plan does not attempt to:

- admit the named-range-combined preserve slice by assumption
- canonicalize away `FormulaGroup` anchors merely for diagnostics
- admit `HostUnknown` listener anchors as a live owned surface
- widen named-range-combined regroup, merge, or member-exit in the same pass
- widen off-sheet or repair-sensitive behavior
- claim ownership of arbitrary Calc listener orchestration outside the
  already-admitted workbook slice

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in listener-anchor contract note
2. a checked-in listener-anchor scenario matrix
3. a checked-in listener-anchor mapping note
4. runtime support for true `FormulaGroup` listener-anchor realization and
   rollback on the admitted slice
5. standalone and live proof for the new listener-anchor support surface
6. rerun evidence on the currently blocked named-range preserve family
7. a checked-in evidence note and decision record

## Workstreams

### 1. Freeze The Listener-Anchor Contract

Define the exact live ownership boundary before changing runtime behavior.

This contract should freeze:

- which listener-anchor kinds become live-owned:
  - `FormulaCell`
  - `FormulaGroup`
- which remain outside contract:
  - `HostUnknown`
- which workbook classes may use the new support first:
  - already-admitted shared-group surfaces
  - blocked bounded preserve follow-on proof such as named-range-combined
    preserve

### 2. Freeze The Scenario Matrix

Define the exact proof buckets the cycle must cover.

This matrix should separate:

- formula-cell-only listener anchors on existing admitted families
- formula-group listener anchors on already-admitted shared-group live paths
- formula-group listener anchors on blocked named-range-combined preserve
- explicit reject buckets for `HostUnknown` listener anchors
- retained reject buckets for off-sheet, repair-sensitive, and broader
  named-range-combined surfaces

### 3. Freeze The Mapping Rules

Define how listener-anchor identity is represented and compared.

This mapping note should state:

- how `FormulaGroup` anchor identity is represented:
  - anchor address
  - group length
- how predicted and live listener edges compare
- what counts as exact
- what remains only diagnostic canonicalization
- where `HostUnknown` still forces reject

### 4. Land Runtime Listener-Anchor Support

Teach live wiring and realization to understand `FormulaGroup` anchors
directly instead of rejecting them.

This workstream should cover:

- live wiring resolution
- admitted wiring-container residency
- object realization
- primitive realization and rollback
- final verification inputs
- mutation-entry result propagation

### 5. Prove Exact Wiring And Verification Closure

Run the bounded proof cycle for the new listener-anchor surface.

This proof must include:

- helper-level proof that `FormulaGroup` listener anchors resolve and apply
- live object-realization proof
- rollback proof
- final verification proof
- retained reject proof for `HostUnknown`

### 6. Re-run The Blocked Admission Candidate

After live listener-anchor support lands, rerun the exact bounded family
that exposed the blocker:

- same-sheet shareable named-range-combined `SameTextPreserve`

This step does not automatically admit it. It only determines whether the
listener-anchor blocker is now removed cleanly enough for a real admission
decision.

### 7. Close Out The Cycle

Write the final evidence and decision record.

The closeout must state explicitly:

- whether true `FormulaGroup` listener-anchor live support is now part of
  the admitted ownership surface
- whether `HostUnknown` remains deferred
- whether the named-range-combined preserve blocker is removed
- what the next adjacent concern becomes after this cycle

## Recommended Execution Order

1. Freeze the listener-anchor contract.
2. Freeze the listener-anchor matrix.
3. Freeze the listener-anchor mapping rules.
4. Land true `FormulaGroup` listener-anchor runtime support.
5. Prove live wiring, realization, rollback, and final verification.
6. Re-run the blocked named-range-combined preserve family.
7. Write the evidence note and decision record.

## Success Criteria

This plan is successful when the repository contains a checked-in result
that makes the blocker smaller and more explicit than it is today.

That means one of the following must be true:

- true `FormulaGroup` listener-anchor support is live-owned on the admitted
  slice with exact proof
- the cycle ends deferred with explicit proof showing which live stage still
  cannot carry that anchor kind

This plan fails if it relaxes verification by treating listener-anchor
mismatch as “good enough” instead of making the live path own it directly.
