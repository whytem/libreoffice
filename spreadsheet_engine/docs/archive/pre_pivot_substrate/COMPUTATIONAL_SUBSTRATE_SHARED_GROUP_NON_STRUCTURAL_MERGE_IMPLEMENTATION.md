# Computational Substrate Shared-Group Non-Structural Merge Implementation

Status: exact merge closeout implemented

## What Landed

This cycle adds one new admitted family on top of the prior member-exit,
same-text preserve, and edge-regroup slice:

- same-sheet shareable shared-group gap-closing merge `SetFormula`

The landed runtime keeps that widening narrow:

- only blank touched addresses are newly eligible
- only `SetFormula` is newly admitted
- the merge family is gap-closing only
- the admitted merge window is exactly the upper group, touched gap cell,
  and lower group
- one-sided adjacent insert and replacement-driven merge remain deferred

## Runtime Changes

The main runtime changes landed in:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)

The closeout behavior is:

- the facade consumer layer now classifies bounded gap-closing shared-group
  merge separately from regroup
- the non-structural shared-group insert defer rule now exempts only the
  bounded two-group gap-merge surface
- the predicted shadow can now create the inserted formula cell before
  topology rebuild
- the engine-authored merge window spans the two adjacent prior shareable
  groups plus the touched gap cell
- shared-group bindings are rebuilt from lowered formulas inside that window
  before exact after-state comparison

## Carry-Through Shape

The admitted merge family uses the same engine-authored carry-through shape
as the earlier admitted non-structural families:

- materialize the predicted after-shadow
- build the dependency snapshot from that predicted facade
- reapply observation state from the predicted dependency and recalc result
- compare against the observed after-state for exact verification

That means exact gap merge now closes through the same computational, queue,
graph, and IR pipeline already used by the admitted member-exit,
same-text preserve, and edge-regroup families.

## What Did Not Land

This implementation intentionally does not admit:

- one-sided adjacent insertion next to only one prior shared group
- replacement-driven merge across a prior shared-group boundary
- multi-group collapse beyond the bounded two-group gap family
- named-range-combined merge
- repair-sensitive host normalization
- off-sheet merge widening
