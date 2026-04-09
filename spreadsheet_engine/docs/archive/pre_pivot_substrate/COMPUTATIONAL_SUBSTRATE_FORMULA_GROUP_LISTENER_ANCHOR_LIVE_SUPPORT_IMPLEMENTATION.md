# Computational Substrate Formula-Group Listener-Anchor Live Support Implementation

Status: completed implementation note for the FormulaGroup listener-anchor live-support closeout

## What Landed

This cycle removed the live wiring hard reject for true `FormulaGroup`
listener anchors on already-admitted shared-group surfaces.

The main runtime work landed in:

- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Runtime Changes

The live wiring path now:

- validates resident listener anchors as either `FormulaCell` or
  `FormulaGroup`
- resolves unique `FormulaGroup` anchors back to live shared-top block
  pointers
- replays those anchors through Calc's shared-group listening helper
- skips redundant per-cell listener-edge reapplication for cells that
  belong to replayed groups
- still rejects `HostUnknown` with
  `listener_anchor_out_of_contract`

No verification rule was weakened. Exactness still depends on the normal
queue, computational, graph, and broadcaster comparisons.

## What The Cycle Changed Operationally

The important operational effect is narrower than admission:

- already-admitted shared-group live paths continue to run exact with true
  `FormulaGroup` listener anchors present
- the bounded named-range-combined preserve rerun no longer fails because
  the listener-anchor kind is unknown to live wiring

The remaining named-range-combined blockers are now later-stage ownership
gaps instead of listener-kind rejection.

## What Did Not Close

The cycle did not widen the admitted named-range-combined slice.

The remaining proven misses are:

- lifecycle `opaque_dependency_surface`
- mutation-entry `rollback_queue_or_state_mismatch`
- standalone synthetic object-realization exact restore still closing as
  `computational_mismatch`
- standalone synthetic rollback exact restore still closing as
  `missing_restored_objects`
