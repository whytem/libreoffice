# Computational Substrate Shared-Group Non-Structural Named-Range-Combined Decision Record

Status: complete closeout decision for the bounded named-range-combined cycle

## Decision

Do not widen the admitted shared-group non-structural slice in this cycle.

The previously admitted non-structural families remain admitted:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`
- same-sheet shareable shared-group same-text preserve `SetFormula`
- same-sheet shareable shared-group edge-regroup `SetFormula`
- same-sheet shareable shared-group gap-closing merge `SetFormula`
- same-sheet shareable shared-group edge replacement-merge `SetFormula`
- same-sheet shareable shared-group one-sided adjacent insertion `SetFormula`

## Why The Cycle Stays Deferred

The cycle proved two different facts:

- bounded named-range-combined same-text preserve closes exactly in
  standalone proof
- live carry-through still does not admit that same family

The live failures are now explicit:

- lifecycle keeps the preserve family out of contract
- mutation entry reaches live-apply and final-verification surfaces that
  still report `listener_anchor_out_of_contract`
- bounded named-range-combined member-exit also stays out of contract

That means the named-range-combined surface is still not engine-owned end to
end on the admitted slice.

## Final Boundary

The admitted non-structural shared-group surface is now:

- member-exit scalar, formula, and clear
- same-text preserve formula replacement on an already-shared same-sheet
  shareable member
- exact same-sheet shareable edge-regroup `SetFormula`
- exact same-sheet shareable gap-closing merge `SetFormula`
- exact same-sheet shareable edge replacement-merge `SetFormula`
- exact same-sheet shareable one-sided adjacent insertion `SetFormula`

The entire named-range-combined frontier remains explicitly outside live
admission.

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_NAMED_RANGE_COMBINED_EVIDENCE.md)

## What This Cycle Added Anyway

Even without widening the slice, this cycle still produced useful
groundwork:

- explicit facade-side named-range-combined boundary classification
- preserve-only named-range candidate gating in the shared-group authority
  builder
- standalone exact proof for the bounded preserve candidate
- explicit live reject coverage for bounded preserve and bounded member-exit

## Next Adjacent Concern

The remaining adjacent concerns are now:

- repair-sensitive host normalization
- off-sheet shared-group behavior
- broader named-range-combined member-exit, regroup, merge, and
  multi-group-collapse classes
- broader non-edge regroup and merge
