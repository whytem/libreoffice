# Computational Substrate Shared-Group Non-Structural Replacement-Merge Implementation

Status: exact replacement-merge closeout implemented

## What Landed

This cycle adds one new admitted family on top of the earlier member-exit,
same-text preserve, edge-regroup, and gap-closing merge slice:

- same-sheet shareable shared-group edge replacement-merge `SetFormula`

The landed runtime keeps that widening narrow:

- only already-shared touched addresses are newly eligible
- only edge members of the touched pre-group are admitted
- only one adjacent prior shareable group may be absorbed
- the admitted rebuild window is the touched pre-group plus that adjacent
  prior group
- untouched remainder from the touched pre-group may stay outside the
  merged after-group if that is what exact lowered rebuild produces

## Runtime Changes

The main runtime changes landed in:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)

The closeout behavior is:

- the facade consumer layer now classifies bounded replacement merge
  separately from gap merge and regroup
- the authority builder can identify the touched prior group plus exactly
  one adjacent prior shareable group
- the predicted shadow selects a bounded replacement-merge rebuild window
  only when the observed-after touched group matches the admitted edge
  pattern
- shared-group bindings are rebuilt from lowered formulas inside that window
  before exact after-state comparison

## Carry-Through Shape

The admitted replacement-merge family uses the same engine-authored
carry-through shape as the earlier admitted non-structural families:

- materialize the predicted after-shadow
- build the dependency snapshot from that predicted facade
- reapply observation state from the predicted dependency and recalc result
- compare against the observed after-state for exact verification

That means exact replacement merge now closes through the same
computational, queue, graph, and IR pipeline already used by the admitted
member-exit, same-text preserve, edge-regroup, and gap-merge families.

## What Did Not Land

This implementation intentionally does not admit:

- one-sided adjacent insertion next to only one prior shared group
- multi-group collapse beyond the bounded two-participant family
- named-range-combined replacement merge
- repair-sensitive host normalization
- off-sheet replacement merge widening
- broader non-edge regroup or merge
