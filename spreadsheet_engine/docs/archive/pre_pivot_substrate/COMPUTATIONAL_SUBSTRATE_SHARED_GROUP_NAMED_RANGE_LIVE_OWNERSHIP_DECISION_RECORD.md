# Computational Substrate Shared-Group Named-Range Live Ownership Decision Record

Status: complete closeout decision for bounded named-range-combined live ownership

## Decision

Widen the admitted shared-group slice by one bounded step.

The admitted slice now includes:

- same-sheet shareable shared-group named-range-combined `SetFormula`
  `SameTextPreserve`
- only when the named-range boundary is
  `GlobalSingleAreaSameSheet`

## Why This Family Is Now Admitted

This cycle removed the exact blocker that previously kept the family
deferred:

- the named-range dependency surface is no longer opaque
- lifecycle now applies exactly on the bounded preserve family
- mutation entry now applies exactly on the same bounded preserve family

The engine now owns that bounded named-range-combined preserve surface end
to end.

## Final Boundary

The admitted shared-group slice now includes:

- structural `Preserve`, `Split`, and `Rebuild`
- non-structural member-exit scalar, formula, and clear
- non-structural same-text preserve
- bounded regroup and merge families admitted in prior cycles
- bounded named-range-combined same-text preserve

The following remain deferred:

- named-range-combined member-exit
- broader named-range-combined regroup, merge, and collapse
- repair-sensitive normalization
- off-sheet shared-group behavior

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_EVIDENCE.md)
