# Computational Substrate Shared-Group Non-Structural Replacement-Merge Decision Record

Status: complete closeout decision for the exact replacement-merge cycle

## Decision

Widen the admitted shared-group non-structural slice by one additional exact
family.

The newly admitted family is:

- same-sheet shareable shared-group edge replacement-merge `SetFormula`

The previously admitted non-structural families remain admitted:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`
- same-sheet shareable shared-group same-text preserve `SetFormula`
- same-sheet shareable shared-group edge-regroup `SetFormula`
- same-sheet shareable shared-group gap-closing merge `SetFormula`

## Why This Family Admits

This family now meets the same standard as the existing admitted slice:

- the after-topology is still engine-authored
- the touched pre-group plus adjacent prior group window is explicit and
  bounded
- the admitted after-group must include the touched address and fully absorb
  the adjacent prior group
- residual untouched formulas from the touched pre-group are handled by the
  same exact lowered rebuild instead of copied host grouping
- lifecycle and mutation-entry exact closure are proven
- the admitted after-state still closes exact computational, queue, graph,
  and IR verification

## Why The Rest Stays Deferred

The replacement-merge closeout still does not prove exact engine-owned
authority for:

- one-sided adjacent insertion next to only one prior shared group
- multi-group merge collapse
- named-range-combined replacement merge
- repair-sensitive host normalization
- off-sheet replacement merge widening
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

The broader non-structural shared-group frontier remains explicitly outside
live admission.

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_EVIDENCE.md)

## Next Adjacent Concern

The next adjacent concern is narrower again:

- whether one-sided adjacent insertion, named-range-combined,
  repair-sensitive, off-sheet, or broader non-edge regroup and merge
  classes can each produce their own exact engine-authored promotion family
