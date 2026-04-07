# Computational Substrate Shared-Group Non-Structural Frontier Decision Record

Status: complete closeout decision for the broader non-structural shared-group frontier cycle

## Decision

Widen the admitted shared-group non-structural slice by one additional exact
family and keep the rest of the frontier deferred.

The new admitted family is:

- same-sheet shareable shared-group same-text preserve `SetFormula`

The previously admitted member-exit family remains admitted:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`

## Why This Family Admits

This family now meets the same standard as the existing admitted slice:

- the after-topology is still engine-authored
- the group identity does not depend on hidden host regrouping or merge
  behavior
- lifecycle and mutation-entry exact closure are proven
- the admitted after-state still closes exact computational, graph, and IR
  verification

The crucial difference from regroup and merge is that same-text preserve does
not require the engine to invent new post-edit group identity. It only has to
prove that the existing identity is preserved exactly.

## Why The Rest Stays Deferred

The broader frontier still does not prove exact engine-owned authority for:

- regroup
- merge
- named-range-combined shared-group behavior
- repair-sensitive host normalization
- off-sheet shared-group behavior

Those classes either still depend on hidden host regrouping semantics or do
not close exactly on the current authority surface.

## Final Boundary

The admitted non-structural shared-group surface is now:

- member-exit scalar, formula, and clear
- same-text preserve formula replacement on an already-shared same-sheet
  shareable group member

The broader frontier remains explicitly outside live admission.

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_EVIDENCE.md)

## Next Adjacent Concern

The next adjacent concern is no longer "can same-text preserve admit?"

It is now narrower again:

- whether regroup, merge, named-range-combined, repair-sensitive, or
  off-sheet classes can each produce their own exact engine-authored proof
  family without borrowing host-observed after-state as authority

That next staged cycle is now complete and closed out in:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md)
