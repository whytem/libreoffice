# Computational Substrate Formula-Group Listener-Anchor Live Support Scenario Matrix

Status: frozen scenario matrix for the FormulaGroup listener-anchor live-support closeout

## Exact Carry-Through Buckets

- existing admitted formula-cell-only live surfaces remain exact
- existing admitted shared-group same-text-preserve lifecycle remains exact
- existing admitted shared-group same-text-preserve mutation entry remains
  exact

## Blocker-Removal Buckets

- bounded named-range-combined same-text preserve lifecycle no longer fails
  with `listener_anchor_out_of_contract`; it now stops at
  `opaque_dependency_surface`
- bounded named-range-combined same-text preserve mutation entry no longer
  fails with `listener_anchor_out_of_contract`; it now stops at
  `rollback_queue_or_state_mismatch`

## Standalone Listener-Anchor Proof Buckets

- standalone shared-group object realization with live `FormulaGroup`
  anchors now applies without out-of-contract reject, but still closes as
  `computational_mismatch`
- standalone shared-group rollback with live `FormulaGroup` anchors now
  applies without out-of-contract reject, but still closes as
  `missing_restored_objects`

## Explicit Reject Buckets

- `HostUnknown` listener anchors still reject with
  `listener_anchor_out_of_contract`
- off-sheet, repair-sensitive, and broader named-range-combined families
  remain outside this cycle
