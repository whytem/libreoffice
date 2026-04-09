# Computational Substrate Shared-Group Non-Structural Replacement-Merge Admission Plan

Status: complete closeout record for the exact replacement-driven merge cycle

## Purpose

This document closes the bounded replacement-driven merge cycle that started
from
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md).

That earlier closeout admitted gap-closing merge and left one adjacent true
merge family unresolved:

- an already-shared edge member `SetFormula` that absorbs exactly one
  adjacent prior shared group

## Closeout Answer

One bounded replacement-driven merge family is now admitted.

The newly admitted family is:

- same-sheet shareable shared-group edge replacement-merge `SetFormula`

That family is exact only when all of the following hold:

- the touched address is already shared before the mutation
- the touched address remains shared after the mutation
- the touched address is the anchor or tail edge of the touched pre-group
- exactly one adjacent same-column shareable prior group participates on the
  touched edge side
- the admitted rebuild window is the touched pre-group plus that adjacent
  prior group
- the engine-authored after-group must include the touched address and fully
  absorb the adjacent prior group
- untouched remainder from the touched pre-group may remain outside the
  merged after-group if the exact lowered rebuild produces that result
- named ranges stay unchanged
- the case stays same-sheet, shareable, and clean-baseline

Representative admitted shapes:

- anchor-edge upward replacement merge:
  - before `B1:B2` shared `*3`, `B3:B4` shared `*2`
  - `SetFormula(B3, "=A3*3")`
  - after `B1:B3` shared `*3`, `B4` ordinary `*2`
- tail-edge downward replacement merge:
  - before `B1:B2` shared `*2`, `B3:B4` shared `*3`
  - `SetFormula(B2, "=A2*3")`
  - after `B1` ordinary `*2`, `B2:B4` shared `*3`

## Landed Deliverables

This closeout now has all required artifacts:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md)

## Deferred Boundary After Closeout

This cycle does not admit:

- one-sided adjacent formula insertion
- multi-group collapse beyond the bounded two-participant replacement family
- named-range-combined replacement merge
- repair-sensitive host-only normalization
- off-sheet replacement merge
- broader non-edge regroup or merge classes
