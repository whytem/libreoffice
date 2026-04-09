# Computational Substrate Shared-Group Non-Structural Merge Admission Plan

Status: complete closeout record for the exact merge cycle

## Closeout Result

This plan is now complete.

The closeout result admits one additional exact family:

- same-sheet shareable shared-group gap-closing merge `SetFormula`

It keeps the remaining merge-adjacent frontier deferred:

- one-sided adjacent insertion next to only one prior shared group
- replacement-driven merge across a prior shared-group boundary
- multi-group merge collapse
- named-range-combined merge
- repair-sensitive host-only normalization
- off-sheet merge widening

The closeout references are now:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md)

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md).

The regroup closeout admitted one additional family:

- same-sheet shareable shared-group edge-regroup `SetFormula`

It also clarified the next blocker:

- exact merge still does not have an engine-authored post-edit group
  identity model when multiple prior participants collapse into one
  shareable after-group

This plan therefore isolates exact merge as the next bounded promotion
question rather than mixing it with named-range-combined, repair-sensitive,
or off-sheet classes.

## Plan Goal

Determine whether one bounded same-sheet shareable shared-group merge family
can move into the admitted slice with exact authority.

The closeout must answer one explicit question:

- can the engine author a merge after-state when a `SetFormula` edit causes
  exactly two prior same-column participants to collapse into one exact
  shareable after-group without borrowing host-observed topology as
  authority

## Entry Boundary

This plan begins from the currently settled shared-group surface:

- structural shared-group `Preserve`, `Split`, and `Rebuild` are admitted
- non-structural member-exit scalar, formula, and clear are admitted
- non-structural same-text preserve `SetFormula` is admitted
- non-structural edge-regroup `SetFormula` is admitted
- merge, named-range-combined, repair-sensitive, off-sheet, and non-edge
  regroup classes remain deferred

The current runtime still keeps merge outside live admission in two explicit
places:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
  rejects adjacent blank-cell `SetFormula` merge candidates through
  `isDeferredSharedGroupNonStructuralFormulaInsert(...)`
- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
  also bounds regroup rebuild windows so they do not absorb another prior
  shared group
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
  routes those merge-shaped inserts to out-of-contract rejection instead of
  ordinary formula insert lifecycle apply
- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
  does not yet expose a dedicated non-structural merge family distinct from
  regroup

## Candidate Merge Slice

The first merge promotion attempt must stay narrow.

The primary candidate slice is:

- same-sheet shareable shared groups only
- `SetFormula` only
- clean baseline only
- touched address is blank before the mutation and formula after the mutation
- exactly two before-groups participate:
  - one shareable same-column group immediately above the touched address
  - one shareable same-column group immediately below the touched address
- both before-groups lower to the same shared-formula identity
- the after-state is exactly one shareable group spanning the combined
  contiguous run
- the touched formula source is compatible with that one merged after-group
- no named-range drift
- no off-sheet dependency expansion beyond the already-admitted surface

Representative shape:

- `B1:B2` is one shareable group
- `B3` is blank
- `B4:B5` is one shareable group
- `SetFormula(B3, ...)` produces one exact shareable group `B1:B5`

This is the recommended first merge family because it matches the current
explicit defer seam and has a clear two-before-to-one-after identity rule.

Optional secondary slice, only if the primary family closes cleanly in the
same cycle:

- touched shared-group edge replacement that absorbs exactly one adjacent
  prior shared group
- still exactly two before participants and one after-group
- still same-sheet, shareable, and clean-baseline only

This plan succeeds if the bounded gap-closing merge admits even if
replacement-driven merge stays deferred.

## Non-Goals

This plan does not attempt to:

- admit multi-gap or multi-group collapse
- admit named-range-combined merge
- admit off-sheet merge
- admit repair-sensitive host-only normalization
- admit interior or non-edge merge windows that require more than two
  before-participants
- reopen already-settled member-exit, same-text preserve, or edge-regroup
  decisions

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in merge contract note
2. a checked-in merge scenario matrix
3. a checked-in merge mapping note
4. facade-side merge classification that is distinct from regroup
5. a dedicated engine-authored merge candidate path
6. standalone and live exact proof or an explicit defer decision
7. a checked-in evidence note and decision record

## Workstreams

### 1. Freeze Exact Merge Semantics

Define the before/after identity rules for:

- two before-groups and one after-group
- gap-closing insert merge versus regroup
- optional replacement-driven merge versus broader multi-group collapse

### 2. Freeze The Merge Matrix

Cover:

- one-row gap merge between two shareable same-column groups
- short and long participant groups
- exact anchor selection for the merged after-group
- reject cases that are really one-sided extension, multi-group collapse,
  named-range-combined, off-sheet, or repair-sensitive

### 3. Land Facade-Side Merge Classification

Freeze a stable merge classification surface through the workbook facade so
tests can identify merge separately from regroup and member-exit.

### 4. Land Engine-Authored Merge Prediction

Teach the non-structural predictor to:

- lower the inserted formula and both adjacent shareable groups
- verify exact merge compatibility across the full combined window
- build one exact predicted after-group without copying observed
  after-topology
- continue to reject merge shapes that remain outside the bounded slice

### 5. Prove Exact Closure

Add:

- facade merge classification proof
- standalone exact merge proof
- live lifecycle merge proof
- live mutation-entry merge proof
- retained reject coverage for broader merge shapes that stay deferred

### 6. Close Out

Record whether exact merge:

- is admitted
- stays validation-only
- or remains deferred

Closeout result:

- admitted for the bounded same-sheet shareable two-group gap-closing
  `SetFormula` merge family
- deferred for one-sided insertion, replacement-driven merge,
  named-range-combined, repair-sensitive, off-sheet, and multi-group merge
  classes
