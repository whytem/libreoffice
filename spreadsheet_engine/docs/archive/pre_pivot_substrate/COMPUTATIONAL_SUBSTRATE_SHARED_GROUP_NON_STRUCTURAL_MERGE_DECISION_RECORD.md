# Computational Substrate Shared-Group Non-Structural Merge Decision Record

Status: complete closeout decision for the exact merge cycle

## Decision

Widen the admitted shared-group non-structural slice by one additional exact
family.

The newly admitted family is:

- same-sheet shareable shared-group gap-closing merge `SetFormula`

The previously admitted non-structural families remain admitted:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`
- same-sheet shareable shared-group same-text preserve `SetFormula`
- same-sheet shareable shared-group edge-regroup `SetFormula`

## Why This Family Admits

This family now meets the same standard as the existing admitted slice:

- the after-topology is still engine-authored
- the two-before-to-one-after merge window is explicit and bounded
- the inserted formula cell is authored inside the predicted shadow before
  exact topology rebuild
- the admitted after-state does not borrow host-observed topology as
  authority
- lifecycle and mutation-entry exact closure are proven
- the admitted after-state still closes exact computational, graph, and IR
  verification

## Why The Rest Stays Deferred

The merge closeout still does not prove exact engine-owned authority for:

- one-sided adjacent insertion next to only one prior shared group
- replacement-driven merge across a prior shared-group boundary
- multi-group merge collapse
- named-range-combined merge
- repair-sensitive host normalization
- off-sheet merge widening
- non-edge regroup outside the already-admitted edge-regroup family

Those classes either still depend on hidden host regrouping semantics or do
not yet have an engine-authored bounded rule family.

## Final Boundary

The admitted non-structural shared-group surface is now:

- member-exit scalar, formula, and clear
- same-text preserve formula replacement on an already-shared same-sheet
  shareable member
- exact same-sheet shareable edge-regroup `SetFormula`
- exact same-sheet shareable gap-closing merge `SetFormula`

The broader non-structural shared-group frontier remains explicitly outside
live admission.

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_EVIDENCE.md)

## Next Adjacent Concern

The replacement-driven merge concern from this record is now closed by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md)

The remaining adjacent concerns are now:

- one-sided adjacent insertion
- named-range-combined merge
- repair-sensitive host normalization
- off-sheet merge widening
- broader non-edge regroup or merge
