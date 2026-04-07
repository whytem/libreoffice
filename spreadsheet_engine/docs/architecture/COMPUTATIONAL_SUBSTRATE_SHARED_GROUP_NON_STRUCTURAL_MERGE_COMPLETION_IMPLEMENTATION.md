# Computational Substrate Shared-Group Non-Structural Merge Completion Implementation

Status: same-sheet merge-completion closeout implemented

## What Landed

This cycle adds one new admitted family on top of the earlier same-sheet
merge slice:

- same-sheet shareable shared-group one-sided adjacent insertion `SetFormula`

The landed runtime keeps that widening narrow:

- only blank touched addresses are newly eligible
- only `SetFormula` is newly admitted
- only one adjacent prior shareable group may participate
- the admitted rebuild window is exactly the inserted cell plus that prior
  group
- broader multi-group collapse remains deferred

## Runtime Changes

The main runtime changes landed in:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)

The closeout behavior is:

- the facade consumer layer now classifies bounded one-sided shared-group
  insert separately from gap merge, replacement merge, regroup, and
  ordinary insert
- the non-structural insert defer rule now exempts the bounded single-group
  one-sided insert surface
- the authority builder can identify exactly one adjacent prior shareable
  participant for a blank touched address
- the engine-authored rebuild window spans only that prior group plus the
  inserted cell
- shared-group bindings are rebuilt from lowered formulas inside that window
  before exact after-state comparison

## Carry-Through Shape

The admitted one-sided insert family uses the same engine-authored
carry-through shape as the earlier admitted non-structural families:

- materialize the predicted after-shadow
- build the dependency snapshot from that predicted facade
- reapply observation state from the predicted dependency and recalc result
- compare against the observed after-state for exact verification

That means exact one-sided insert now closes through the same
computational, queue, graph, and IR pipeline already used by the admitted
member-exit, same-text preserve, edge-regroup, gap-merge, and
replacement-merge families.

## What Did Not Land

This implementation intentionally does not admit:

- multi-group collapse beyond the current two-participant merge families
- named-range-combined merge
- repair-sensitive host normalization
- off-sheet merge widening
- broader non-edge regroup or merge
