# Computational Substrate Shared-Group Non-Structural Regroup Implementation

Status: exact regroup closeout implemented

## What Landed

This cycle adds one new admitted family on top of the prior member-exit and
same-text preserve slice:

- same-sheet shareable shared-group edge-regroup `SetFormula`

The landed runtime keeps that widening narrow:

- only already-shared touched addresses are eligible
- only changed-group `SetFormula` is newly admitted
- regroup authoring stays bounded to the touched pre-mutation group plus a
  contiguous adjacent ordinary-formula run
- prior shared-group merge remains deferred

## Runtime Changes

The main runtime changes landed in:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)

The closeout behavior is:

- the non-structural shared-group candidate gate now allows changed-group
  `SetFormula` candidates when the touched address still remains shared
  afterward
- the engine-authored regroup window now extends beyond the touched
  pre-mutation group only through adjacent ordinary formulas that lower
  joinably with the touched after-formula
- the regroup window refuses to absorb another prior shared group, which
  keeps merge deferred
- the predicted after-shadow still rebuilds all shared-group bindings from
  lowered formulas before exact after-state comparison

## Carry-Through Shape

The admitted regroup family uses the same engine-authored carry-through
shape as the earlier admitted non-structural families:

- materialize the predicted after-shadow
- build the dependency snapshot from that predicted facade
- reapply observation state from the predicted dependency and recalc result
- compare against the observed after-state for exact verification

That means exact regroup now closes through the same computational, queue,
graph, and IR pipeline already used by the admitted member-exit and
same-text preserve families.

## What Did Not Land

This implementation intentionally does not admit:

- merge across prior shared groups
- blank-cell insertion adjacent to a shared group
- named-range-combined regroup
- repair-sensitive host normalization
- off-sheet regroup widening
- interior regroup without an engine-authored edge window

