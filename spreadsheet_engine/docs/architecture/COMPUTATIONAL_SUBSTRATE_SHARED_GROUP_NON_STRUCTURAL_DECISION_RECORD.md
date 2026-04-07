# Computational Substrate Shared-Group Non-Structural Decision Record

Status: complete closeout decision for the non-structural shared-group admission cycle

## Decision

Widen the admitted shared-group slice by one bounded non-structural family.

The bounded promotion result is:

- admit same-sheet shareable shared-group member-exit `SetScalarValue`
  through the authority lane
- admit same-sheet shareable shared-group member-exit `SetFormula` and
  `ClearCell` through the lifecycle lane
- admit the same bounded family through mutation entry when both the
  mutation-entry gate and the dedicated non-structural shared-group gate are
  enabled

## Why This Slice Now Admits

The cycle now establishes the exact condition that was still missing after
the structural shared-group widening closeout:

- exact engine-authored after-topology for the bounded non-structural
  member-exit family
- exact live closure through queue, computational, graph, and IR
  verification
- exact mutation-entry carry-through without rejecting resident shared
  formulas already present in the live document

The key boundary shift is specific:

- the predictor removes the touched member from the pre-mutation group
- surviving members are repartitioned into contiguous runs
- only runs longer than one cell remain shared groups
- observed-after topology is used only as an exact validation check, not as
  the admitted after-topology source

That is enough for one bounded non-structural promotion into the admitted
slice.

## Final Boundary

The admitted shared-group surface now becomes:

- exact same-sheet shareable structural shared-group `Preserve`, `Split`, and
  `Rebuild`
- exact same-sheet shareable non-structural shared-group member-exit
  `SetScalarValue`
- exact same-sheet shareable non-structural shared-group member-exit
  `SetFormula`
- exact same-sheet shareable non-structural shared-group member-exit
  `ClearCell`

The non-structural widening is still explicitly bounded by:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP_NON_STRUCTURAL=1`

It does not replace the existing rollout or mutation-entry gates; it only
widens the shared-group slice inside those already-bounded surfaces.

## What This Cycle Still Does Not Establish

The cycle still does not establish broad non-structural shared-group
admission.

What remains unsettled:

- same-text preserve replacements that intentionally retain the group
- regroup or merge behavior across prior groups
- named-range-combined shared-group behavior
- repair-sensitive host-only regrouping or normalization
- off-sheet or broader workbook classes

Those classes do not yet have the same engine-authored exact closure as the
admitted member-exit family.

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md)

## Next Adjacent Concern

The next adjacent concern should stay narrower than another broad
shared-group push.

The right next question is:

- whether regroup, merge, named-range-combined, or broader repair-sensitive
  shared-group classes can be expressed without falling back to retained
  host-only behavior

That next staged cycle is now captured in:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_ADMISSION_PLAN.md)
