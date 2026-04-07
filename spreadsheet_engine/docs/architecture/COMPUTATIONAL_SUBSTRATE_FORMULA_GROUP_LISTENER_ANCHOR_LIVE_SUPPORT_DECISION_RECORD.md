# Computational Substrate Formula-Group Listener-Anchor Live Support Decision Record

Status: complete closeout decision for the FormulaGroup listener-anchor live-support cycle

## Decision

Keep the admitted workbook slice unchanged, but adopt true
`FormulaGroup` listener-anchor live support on the already-admitted
shared-group path.

This cycle does not admit bounded named-range-combined preserve.

## What Changed

Before this cycle, the bounded named-range-combined preserve rerun still
stopped at `listener_anchor_out_of_contract`.

After this cycle:

- `FormulaGroup` listener anchors are live-owned in resident wiring replay
  for the already-admitted shared-group slice
- `HostUnknown` is still rejected
- bounded named-range-combined preserve no longer fails because the
  listener-anchor kind is unsupported

## Why The Admitted Slice Does Not Widen Yet

Removing the listener-anchor blocker was necessary, but it was not enough to
admit the named-range-combined preserve family.

The remaining blockers are now explicit:

- lifecycle stops at `opaque_dependency_surface`
- mutation entry stops at `rollback_queue_or_state_mismatch`
- synthetic standalone exact-restore proof buckets still do not close
  exactly

That means the engine now owns the listener-anchor kind itself, but it still
does not own the full named-range-combined live surface end to end.

## Final Boundary

The admitted shared-group slice remains:

- structural `Preserve`, `Split`, and `Rebuild`
- non-structural member-exit scalar, formula, and clear
- non-structural same-text preserve
- bounded regroup and merge families already admitted in prior cycles

The new live-owned support surface added by this cycle is:

- true `FormulaGroup` listener-anchor replay on that already-admitted live
  slice

The following remain deferred:

- named-range-combined preserve admission
- named-range-combined member-exit, regroup, merge, and collapse
- repair-sensitive host normalization
- off-sheet shared-group behavior

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_GROUP_LISTENER_ANCHOR_LIVE_SUPPORT_EVIDENCE.md)

## Next Adjacent Concern

The next adjacent concern is no longer listener-anchor kind support.

It is now the remaining live ownership gap beyond that support:

- named-range-combined opaque dependency surfaces and exact
  rollback/final-verification closure
- repair-sensitive host normalization
- off-sheet shared-group behavior
