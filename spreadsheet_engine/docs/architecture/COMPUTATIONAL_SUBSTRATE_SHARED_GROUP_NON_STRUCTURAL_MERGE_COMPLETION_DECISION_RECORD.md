# Computational Substrate Shared-Group Non-Structural Merge Completion Decision Record

Status: complete closeout decision for the same-sheet merge-completion cycle

## Decision

Widen the admitted shared-group non-structural slice by one additional exact
family.

The newly admitted family is:

- same-sheet shareable shared-group one-sided adjacent insertion `SetFormula`

The previously admitted non-structural families remain admitted:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`
- same-sheet shareable shared-group same-text preserve `SetFormula`
- same-sheet shareable shared-group edge-regroup `SetFormula`
- same-sheet shareable shared-group gap-closing merge `SetFormula`
- same-sheet shareable shared-group edge replacement-merge `SetFormula`

## Why This Family Admits

This family now meets the same standard as the existing admitted slice:

- the after-topology is still engine-authored
- the inserted-cell-plus-adjacent-group window is explicit and bounded
- the admitted after-state does not borrow host-observed topology as
  authority
- lifecycle and mutation-entry exact closure are proven
- the admitted after-state still closes exact computational, queue, graph,
  and IR verification

## Why The Rest Stays Deferred

The merge-completion closeout still does not prove exact engine-owned
authority for:

- multi-group collapse beyond the current two-participant merge families
- named-range-combined merge
- repair-sensitive host normalization
- off-sheet merge widening
- broader non-edge regroup or merge

Those classes either still depend on hidden host regrouping semantics or do
not yet have an engine-authored bounded rule family.

## Final Boundary

The admitted non-structural shared-group surface is now:

- member-exit scalar, formula, and clear
- same-text preserve formula replacement on an already-shared same-sheet
  shareable member
- exact same-sheet shareable edge-regroup `SetFormula`
- exact same-sheet shareable gap-closing merge `SetFormula`
- exact same-sheet shareable edge replacement-merge `SetFormula`
- exact same-sheet shareable one-sided adjacent insertion `SetFormula`

The broader non-structural shared-group frontier remains explicitly outside
live admission.

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_EVIDENCE.md)

## Next Adjacent Concern

The next adjacent concern is now:

- whether multi-group collapse, named-range-combined,
  repair-sensitive, off-sheet, or broader non-edge regroup and merge
  classes can each produce their own exact engine-authored promotion family
