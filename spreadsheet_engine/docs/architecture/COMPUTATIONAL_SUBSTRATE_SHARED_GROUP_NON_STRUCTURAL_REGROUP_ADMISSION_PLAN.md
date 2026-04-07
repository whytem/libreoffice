# Computational Substrate Shared-Group Non-Structural Regroup Admission Plan

Status: active implementation plan

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_DECISION_RECORD.md).

The frontier closeout admitted one additional family:

- same-sheet shareable shared-group same-text preserve `SetFormula`

It also clarified the next blocker:

- exact regroup still does not have an engine-authored post-edit group
  identity model

This plan therefore isolates exact regroup as the next bounded promotion
question rather than mixing it with merge, named-range, repair-sensitive, or
off-sheet classes.

## Plan Goal

Determine whether one bounded same-sheet shareable shared-group regroup
family can move into the admitted slice with exact authority.

The closeout must answer one explicit question:

- can the engine author a regroup after-state when a touched shared-group
  formula replacement keeps the touched cell shared but changes its group
  identity

## Entry Boundary

This plan begins from the currently settled shared-group surface:

- structural shared-group `Preserve`, `Split`, and `Rebuild` are admitted
- non-structural member-exit scalar, formula, and clear are admitted
- non-structural same-text preserve `SetFormula` is admitted
- regroup, merge, named-range-combined, repair-sensitive, and off-sheet
  classes remain deferred

The current runtime explicitly rejects regroup-shaped same-sheet formula
replacement instead of admitting it as ordinary lifecycle apply.

## Candidate Regroup Slice

The first regroup promotion attempt must stay narrow.

The candidate slice is:

- same-sheet shareable shared groups only
- `SetFormula` only
- touched address already shared before the mutation
- touched address still shared after the mutation
- exactly one touched before-group
- exactly one after-group involving the touched address
- regroup only:
  - one before-group maps to one different after-group
  - no multi-group collapse
  - no blank-cell insertion into a gap
  - no merge of separate before-groups
- no named-range drift
- no off-sheet dependency expansion beyond the already-admitted surface
- clean baseline only

Representative shape:

- one ordinary adjacent formula plus one shareable touched group
- replacement formula causes the touched address to regroup with the adjacent
  formula, producing a different anchor or member set

## Non-Goals

This plan does not attempt to:

- admit merge
- admit adjacent blank-cell insertion into a group
- admit named-range-combined regroup
- admit off-sheet regroup
- admit repair-sensitive host-only normalization
- reopen same-text preserve or member-exit decisions

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in regroup contract note
2. a checked-in regroup scenario matrix
3. a checked-in regroup mapping note
4. a dedicated engine-authored regroup candidate path
5. standalone and live exact proof or an explicit defer decision
6. a checked-in evidence note and decision record

## Workstreams

### 1. Freeze Exact Regroup Semantics

Define the before/after identity rules for:

- touched group before
- touched group after
- what counts as regroup versus preserve or merge

### 2. Freeze The Regroup Matrix

Cover:

- anchor regroup
- interior regroup
- tail regroup
- regroup that changes anchor
- regroup that changes member set without merge
- reject cases that are really merge or blank-cell insertion

### 3. Land Facade-Side Regroup Classification

Freeze a stable regroup classification surface through the workbook facade so
tests can talk about regroup identity directly.

### 4. Land Engine-Authored Regroup Prediction

Teach the non-structural predictor to:

- mutate the touched cell formula
- rebuild group bindings in the bounded regroup window
- produce the exact post-edit touched-group identity without copying
  observed after-topology

### 5. Prove Exact Closure

Add:

- standalone regroup proof
- live lifecycle regroup proof
- live mutation-entry regroup proof
- reject coverage for shapes that are actually merge

### 6. Close Out

Record whether exact regroup:

- is admitted
- stays validation-only
- or remains deferred
