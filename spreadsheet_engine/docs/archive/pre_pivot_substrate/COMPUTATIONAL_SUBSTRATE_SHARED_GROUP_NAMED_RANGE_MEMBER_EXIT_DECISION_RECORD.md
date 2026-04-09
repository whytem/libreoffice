# Computational Substrate Shared-Group Named-Range Member-Exit Decision Record

Status: complete closeout decision for bounded named-range-combined scalar member-exit admission

## Decision

Widen the admitted shared-group slice by one bounded step.

The admitted slice now includes:

- same-sheet shareable shared-group named-range-combined
  `SetScalarValue` `MemberExit`
- only when the named-range boundary is
  `GlobalSingleAreaSameSheet`

## Why This Family Is Now Admitted

This cycle removed the exact remaining blocker on the bounded scalar
member-exit family:

- live graph verification now uses the same canonical graph projection as
  final comparison
- live IR verification now uses Calc-hosted IR compiled from the predicted
  computational after-shadow
- authority and mutation entry both apply exactly on the bounded scalar
  member-exit lane

## Final Boundary

The admitted shared-group slice now includes:

- structural `Preserve`, `Split`, and `Rebuild`
- non-structural member-exit scalar, formula, and clear on the plain
  same-sheet shareable slice
- bounded named-range-combined `SameTextPreserve`
- bounded named-range-combined scalar `MemberExit`
- bounded regroup and merge families admitted in prior cycles

The following remain deferred:

- named-range-combined `SetFormula` member-exit
- named-range-combined `ClearCell` member-exit
- broader named-range-combined regroup, merge, and collapse
- repair-sensitive normalization
- off-sheet shared-group behavior

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_MEMBER_EXIT_EVIDENCE.md)
