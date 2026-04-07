# Computational Substrate Shared-Group Non-Structural Multi-Group Collapse Admission Plan

Status: staged execution plan for the exact multi-group collapse cycle

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_DECISION_RECORD.md).

That closeout admitted one-sided adjacent insertion and left the next
same-sheet merge-shaped blocker unresolved:

- exact multi-group collapse beyond the current two-participant merge
  families

This plan isolates multi-group collapse as the next bounded promotion
question rather than mixing it with named-range-combined, repair-sensitive,
off-sheet, or broader non-edge regroup and merge classes.

## Plan Goal

Determine whether one bounded same-sheet shareable shared-group multi-group
collapse family can move into the admitted slice with exact authority.

The closeout must answer one explicit question:

- can the engine author an exact after-topology when a touched shared-group
  formula replacement collapses exactly three adjacent prior shared groups
  into one after-group without borrowing host-observed topology as
  authority

## Entry Boundary

This plan begins from the currently settled shared-group surface:

- structural shared-group `Preserve`, `Split`, and `Rebuild` are admitted
- non-structural member-exit scalar, formula, and clear are admitted
- non-structural same-text preserve `SetFormula` is admitted
- non-structural edge-regroup `SetFormula` is admitted
- non-structural gap-closing merge `SetFormula` is admitted
- non-structural edge replacement-merge `SetFormula` is admitted
- non-structural one-sided adjacent insertion `SetFormula` is admitted
- multi-group collapse, named-range-combined, repair-sensitive, off-sheet,
  and broader non-edge regroup or merge classes remain deferred

The current runtime still rejects broader merge collapse instead of
admitting it as ordinary lifecycle apply.

## Candidate Collapse Slice

The first multi-group-collapse promotion attempt must stay narrow.

The candidate slice is:

- same-sheet shareable shared groups only
- `SetFormula` only
- touched address already shared before the mutation
- touched address still shared after the mutation
- exactly three adjacent same-column shareable prior groups participate
- the touched address belongs to the middle participant group before the
  mutation
- the admitted rebuild window is the full span of those three prior groups
- the engine-authored after-state must collapse that full span into exactly
  one after-group
- no blank cells, ordinary-formula-only runs, or non-contiguous segments are
  part of the admitted participant set
- no named-range drift
- no off-sheet dependency expansion beyond the already-admitted surface
- clean baseline only

Representative target shape:

- before `B1:B2` shared `*3`, `B3:B4` shared `*2`, `B5:B6` shared `*3`
- `SetFormula(B3, "=A3*3")`
- after `B1:B6` is one exact shareable group

## Non-Goals

This plan does not attempt to:

- admit collapse of four or more prior groups
- admit collapse that includes ordinary-formula-only runs
- admit blank-gap insertion as part of the same cycle
- admit named-range-combined multi-group collapse
- admit off-sheet multi-group collapse
- admit repair-sensitive host-only normalization
- reopen the admitted one-sided, replacement-merge, gap-merge, or regroup
  decisions

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in multi-group-collapse contract note
2. a checked-in multi-group-collapse scenario matrix
3. a checked-in multi-group-collapse mapping note
4. facade-side classification or neighborhood proof for the bounded
   three-participant collapse family
5. a dedicated engine-authored multi-group-collapse candidate path
6. standalone and live exact proof or an explicit defer decision
7. a checked-in evidence note and decision record

## Workstreams

### 1. Freeze Exact Collapse Semantics

Define the before/after identity rules for:

- the three prior participant groups
- the touched middle group
- the exact one-group after-topology
- what counts as multi-group collapse versus replacement-merge,
  one-sided-insert, or broader regroup

### 2. Freeze The Collapse Matrix

Cover:

- middle-group anchor rewrite that collapses upward and downward neighbors
- middle-group interior rewrite if Calc still produces one after-group
- reject cases that are really two-participant merge
- reject cases that rely on blank insertion
- reject four-plus-group collapse
- reject named-range, repair-sensitive, and off-sheet intersections

### 3. Land Facade-Side Collapse Identification

Freeze a stable facade-side description of the bounded three-participant
collapse so tests can talk about:

- participant count
- touched-group position
- after-group span

without inferring those facts ad hoc in each proof.

### 4. Land Engine-Authored Collapse Prediction

Teach the non-structural predictor to:

- discover exactly three adjacent participant groups around the touched
  middle group
- build one bounded rebuild window spanning all three groups
- mutate the touched formula inside the predicted shadow
- rebuild shared-group bindings from lowered formulas across the full window
- admit only an exact one-group after-topology for that bounded class

### 5. Prove Exact Closure

Add:

- standalone three-participant collapse proof
- live lifecycle three-participant collapse proof
- live mutation-entry three-participant collapse proof
- retained reject coverage for four-plus-group collapse
- retained reject coverage for named-range, repair-sensitive, and off-sheet
  intersections

### 6. Close Out

Record whether bounded exact multi-group collapse:

- is admitted
- stays validation-only
- or remains deferred

If the exact authored model naturally generalizes to a slightly broader
same-sheet shareable family without adding new authority concepts, document
that explicitly. Otherwise keep the closeout at the three-participant slice.

## Recommended Execution Order

The safest order for this cycle is:

1. freeze the contract, scenario matrix, and mapping rules
2. land facade-side participant identification
3. land the three-participant engine-authored rebuild window
4. prove standalone exact closure
5. prove live lifecycle and mutation-entry closure
6. decide whether any strictly larger same-sheet family closes on the same
   model
7. write evidence and the final decision record

## Success Criteria

This plan should be considered successful only if the final closeout can say
all of the following for the newly admitted family:

- the participant set is engine-authored and bounded
- the after-group topology is engine-authored rather than copied from Calc
- queue, computational, graph, and IR verification close exactly
- lifecycle and mutation-entry both behave deterministically on the admitted
  class
- four-plus-group, named-range-combined, repair-sensitive, off-sheet, and
  broader non-edge regroup or merge classes still stay obviously outside the
  admitted slice unless they also independently close
