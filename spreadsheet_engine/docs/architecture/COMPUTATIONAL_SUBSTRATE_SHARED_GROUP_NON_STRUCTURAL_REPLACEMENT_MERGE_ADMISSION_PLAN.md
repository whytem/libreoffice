# Computational Substrate Shared-Group Non-Structural Replacement-Merge Admission Plan

Status: active implementation plan for the exact replacement-driven merge cycle

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md).

The merge closeout admitted one additional family:

- same-sheet shareable shared-group gap-closing merge `SetFormula`

It also clarified the next blocker:

- exact replacement-driven merge still does not have an engine-authored
  post-edit group identity model when a touched shared-group edge member
  absorbs exactly one adjacent prior shared group

This plan therefore isolates replacement-driven merge as the next bounded
promotion question rather than mixing it with named-range-combined,
repair-sensitive, off-sheet, or broader regroup classes.

## Plan Goal

Determine whether one bounded same-sheet shareable shared-group
replacement-driven merge family can move into the admitted slice with exact
authority.

The closeout must answer one explicit question:

- can the engine author a merge after-state when a touched already-shared
  edge member `SetFormula` causes exactly one adjacent prior shared group to
  collapse into the touched group’s exact after-group without borrowing
  host-observed topology as authority

## Entry Boundary

This plan begins from the currently settled shared-group surface:

- structural shared-group `Preserve`, `Split`, and `Rebuild` are admitted
- non-structural member-exit scalar, formula, and clear are admitted
- non-structural same-text preserve `SetFormula` is admitted
- non-structural edge-regroup `SetFormula` is admitted
- non-structural gap-closing merge `SetFormula` is admitted
- replacement-driven merge, one-sided adjacent insertion,
  named-range-combined, repair-sensitive, off-sheet, and non-edge regroup
  classes remain deferred

The current runtime already has two relevant bounded seams:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
  can now author gap-closing merge windows and still rejects broader merge
  shapes
- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
  still bounds regroup windows so they do not absorb another prior shared
  group
- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
  can distinguish bounded gap merge from regroup, but does not yet expose a
  dedicated replacement-driven merge family

## Candidate Replacement-Merge Slice

The first replacement-driven merge promotion attempt must stay narrow.

The primary candidate slice is:

- same-sheet shareable shared groups only
- `SetFormula` only
- clean baseline only
- touched address is already shared before the mutation
- touched address remains shared after the mutation
- touched address is an edge member of the touched pre-mutation group:
  - anchor edge merge
  - tail edge merge
- exactly two before-groups participate:
  - the touched pre-mutation shareable group
  - one adjacent same-column shareable group on the touched edge side
- the adjacent participant must be contiguous with the touched edge
- the after-state is exactly one shareable group spanning the combined
  contiguous run
- no named-range drift
- no off-sheet dependency expansion beyond the already-admitted surface

Representative shapes:

- anchor replacement merge:
  - `B1:B2` is one shareable group
  - `B3:B4` is the touched shareable group
  - `SetFormula(B3, ...)` produces one exact shareable group `B1:B4`
- tail replacement merge:
  - `B1:B2` is the touched shareable group
  - `B3:B4` is one adjacent shareable group
  - `SetFormula(B2, ...)` produces one exact shareable group `B1:B4`

This is the recommended next family because it is the smallest remaining
true merge class that stays on the already-proven same-sheet/shareable
authority surface.

## Non-Goals

This plan does not attempt to:

- admit one-sided adjacent insertion
- admit multi-group collapse beyond two before-participants
- admit named-range-combined replacement merge
- admit off-sheet replacement merge
- admit repair-sensitive host-only normalization
- admit non-edge regroup or non-edge merge windows
- reopen already-settled member-exit, same-text preserve, edge-regroup, or
  gap-closing merge decisions

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in replacement-merge contract note
2. a checked-in replacement-merge scenario matrix
3. a checked-in replacement-merge mapping note
4. facade-side replacement-merge classification distinct from regroup and
   gap merge
5. a dedicated engine-authored replacement-merge candidate path
6. standalone and live exact proof or an explicit defer decision
7. a checked-in evidence note and decision record

## Workstreams

### 1. Freeze Exact Replacement-Merge Semantics

Define the before/after identity rules for:

- one touched prior group plus one adjacent prior group
- anchor-edge and tail-edge replacement merge
- replacement-driven merge versus edge regroup
- replacement-driven merge versus broader multi-group collapse

### 2. Freeze The Replacement-Merge Matrix

Cover:

- anchor-edge replacement merge
- tail-edge replacement merge
- exact anchor selection for the merged after-group
- exact residual rejection when the candidate would really be regroup,
  one-sided extension, multi-group collapse, named-range-combined,
  off-sheet, or repair-sensitive

### 3. Land Facade-Side Replacement-Merge Classification

Freeze a stable replacement-merge classification surface through the
workbook facade so tests can identify it separately from regroup, gap merge,
and member-exit.

### 4. Land Engine-Authored Replacement-Merge Prediction

Teach the non-structural predictor to:

- recognize the bounded touched-group-plus-adjacent-group replacement-merge
  window
- lower the touched edited formula and both prior participant groups
- verify exact merge compatibility across the full combined window
- build one exact predicted after-group without copying observed
  after-topology
- continue to reject replacement-driven merge shapes that remain outside the
  bounded slice

### 5. Prove Exact Closure

Add:

- facade replacement-merge classification proof
- standalone exact replacement-merge proof
- live lifecycle replacement-merge proof
- live mutation-entry replacement-merge proof
- retained reject coverage for one-sided insertion and broader merge shapes

### 6. Close Out

Record whether exact replacement-driven merge:

- is admitted
- stays validation-only
- or remains deferred

Preferred closeout target:

- admitted for the bounded same-sheet shareable edge replacement-driven
  `SetFormula` merge family
- deferred for one-sided insertion, named-range-combined,
  repair-sensitive, off-sheet, non-edge, and multi-group merge classes
