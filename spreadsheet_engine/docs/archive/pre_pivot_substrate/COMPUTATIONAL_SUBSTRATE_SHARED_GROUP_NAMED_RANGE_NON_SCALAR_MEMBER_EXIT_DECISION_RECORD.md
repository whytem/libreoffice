# Computational Substrate Shared-Group Named-Range Non-Scalar Member-Exit Decision Record

Status: complete closeout decision for bounded named-range-combined
non-scalar member-exit admission

## Decision

Widen the admitted shared-group slice by one bounded step.

The admitted slice now includes:

- same-sheet shareable shared-group named-range-combined `SetFormula`
  `MemberExit`
- only when the named-range boundary is
  `GlobalSingleAreaSameSheet`

## Why `SetFormula` Is Now Admitted

This cycle proved that bounded named-range-combined `SetFormula`
`MemberExit` now closes exactly:

- standalone predicted-after computational, graph, and IR state closes
- live authority applies on the same bounded lane
- live lifecycle applies on the same bounded lane
- live mutation entry applies on the same bounded lane

## Why `ClearCell` Is Still Deferred

The adjacent bounded `ClearCell` lane still does not close exactly on the
live admitted path.

The deciding evidence is narrower than host cleanup:

- live Calc broadcaster observation is already exact after the clear
- the remaining mismatch is the planner-authored predicted broadcaster-node
  and dependency-edge graph surface

So `ClearCell` stays deferred until the authority planner can author that
after-graph exactly.

## Final Boundary

The admitted shared-group slice now includes:

- structural `Preserve`, `Split`, and `Rebuild`
- plain same-sheet shareable non-structural member-exit scalar, formula,
  and clear
- bounded named-range-combined `SameTextPreserve`
- bounded named-range-combined scalar `MemberExit`
- bounded named-range-combined `SetFormula` `MemberExit`
- bounded regroup and merge families admitted in prior cycles

The following remain deferred:

- bounded named-range-combined `ClearCell` `MemberExit`
- broader named-range-combined regroup, merge, and collapse
- repair-sensitive normalization
- off-sheet shared-group behavior

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_NON_SCALAR_MEMBER_EXIT_EVIDENCE.md)
