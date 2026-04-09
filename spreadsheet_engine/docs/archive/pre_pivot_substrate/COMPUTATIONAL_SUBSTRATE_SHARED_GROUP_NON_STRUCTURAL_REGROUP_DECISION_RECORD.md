# Computational Substrate Shared-Group Non-Structural Regroup Decision Record

Status: complete closeout decision for the exact regroup cycle

## Decision

Widen the admitted shared-group non-structural slice by one additional exact
family.

The newly admitted family is:

- same-sheet shareable shared-group edge-regroup `SetFormula`

The previously admitted non-structural families remain admitted:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`
- same-sheet shareable shared-group same-text preserve `SetFormula`

## Why This Family Admits

This family now meets the same standard as the existing admitted slice:

- the after-topology is still engine-authored
- the regroup window is explicit and bounded
- the admitted after-state does not borrow host-observed topology as
  authority
- lifecycle and mutation-entry exact closure are proven
- the admitted after-state still closes exact computational, graph, and IR
  verification

The key difference from merge is explicit:

- edge regroup only absorbs adjacent ordinary formulas
- it does not absorb another prior shared group

## Why The Rest Stays Deferred

The regroup closeout still does not prove exact engine-owned authority for:

- merge across prior shared groups
- blank-gap insertion into a shared group boundary
- named-range-combined regroup
- repair-sensitive host normalization
- off-sheet regroup widening
- interior regroup outside the admitted edge window

Those classes either still depend on hidden host regrouping semantics or do
not yet have an engine-authored bounded rule family.

## Final Boundary

The admitted non-structural shared-group surface is now:

- member-exit scalar, formula, and clear
- same-text preserve formula replacement on an already-shared same-sheet
  shareable member
- exact same-sheet shareable edge-regroup `SetFormula`

The broader regroup and merge frontier remains explicitly outside live
admission.

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_EVIDENCE.md)

## Next Adjacent Concern

The next staged cycle is now complete and closed out in:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md)

The remaining adjacent concern is now narrower again:

- whether replacement-driven merge, named-range-combined,
  repair-sensitive, off-sheet, or broader regroup classes can each produce
  their own exact engine-authored promotion family without widening beyond
  the admitted edge-regroup plus gap-merge slice
